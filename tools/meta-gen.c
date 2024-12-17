#include <tinydir.h>
#include <jsmn.h>

#include "json.h"
#include "sys-cli.c"
#include "mem-arena.c"
#include "mem.c"

#include "str.h"
#include "str.c"
#include "path.c"

#include "io.h"
#include "mem-arena.h"
#include "sys-utils.h"
#include "sys.h"

#define TABLE_EXT ".luntable"

enum COLUMN_TYPE {
	COLUMN_TYPE_NONE,
	COLUMN_TYPE_ID,
	COLUMN_TYPE_LABEL,
	COLUMN_TYPE_BITMASK,
};

struct column {
	str8 name;
	enum COLUMN_TYPE type;
};

struct columns {
	size_t len;
	size_t cap;
	struct column *items;
};

struct row_value {
	enum COLUMN_TYPE type;
	union {
		uint32_t u32;
		str8 string;
	};
};

struct row {
	size_t len;
	struct row_value *items;
};

struct rows {
	size_t len;
	size_t cap;
	struct row *items;
};

struct table {
	str8 name;
	str8 prefix;
	struct columns columns;
	struct rows rows;
};

i32
gen_table(const str8 in_path, struct alloc scratch)
{
	str8 out_file_path = make_file_name_with_ext(scratch, in_path, str8_lit("h"));

	str8 json = {0};
	json_load(in_path, scratch, &json);
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 JSON tokens */
	jsmn_init(&p);
	i32 r = jsmn_parse(&p, (char *)json.str, json.size, t, 128); // "s" is the char array holding the json content
	if(r < 0) {
		sys_printf("Failed to parse JSON: %d\n", r);
		return EXIT_FAILURE;
	}

	sys_printf("tokens: %d", r);
	sys_printf("%s -> %s \n", in_path.str, out_file_path.str);

	struct table table = {0};
	/* Loop over all keys of the root object */
	for(i32 i = 1; i < r; i++) {
		jsmntok_t *key   = &t[i];
		jsmntok_t *value = &t[i + 1];

		if(json_eq(json, key, str8_lit("name")) == 0) {
			assert(value->type == JSMN_STRING);
			table.name = json_str8_cpy_push(json, value, scratch);
			i++;
		} else if(json_eq(json, key, str8_lit("prefix")) == 0) {
			assert(value->type == JSMN_STRING);
			table.prefix = json_str8_cpy_push(json, value, scratch);
			i++;
		} else if(json_eq(json, key, str8_lit("columns")) == 0) {
			assert(value->type == JSMN_ARRAY);

			table.columns.items = malloc(sizeof(struct column) * value->size);
			table.columns.cap   = value->size;
			for(i32 j = 0; j < value->size; j++) {
				jsmntok_t *child_key   = &t[i + j + 1];
				jsmntok_t *child_value = &t[i + j + 2];
				size_t len             = child_value->end - child_value->start;

				struct column column = {0};
				column.name          = json_str8_cpy_push(json, child_value, scratch);
				if(json_eq(json, child_value, str8_lit("id")) == 0) {
					column.type = COLUMN_TYPE_ID;
				} else if(json_eq(json, child_value, str8_lit("label")) == 0) {
					column.type = COLUMN_TYPE_LABEL;
				} else if(json_eq(json, child_value, str8_lit("bitmask")) == 0) {
					column.type = COLUMN_TYPE_BITMASK;
				} else {
					assert(0);
				}
				table.columns.items[table.columns.len++] = column;
			}
			i += value->size + 1;
		} else if(json_eq(json, key, str8_lit("elements")) == 0) {
			assert(value->type == JSMN_ARRAY);
			table.rows.cap   = value->size;
			table.rows.items = malloc(sizeof(struct row) * value->size);

			for(i32 j = 0; j < value->size; j++) {
				struct row row         = {0};
				row.len                = table.columns.len;
				row.items              = malloc(sizeof(struct row_value) * row.len);
				jsmntok_t *child_key   = &t[i + j + 1];
				jsmntok_t *child_value = &t[i + j + 2];
				size_t len             = child_value->end - child_value->start;
				assert(child_value->type == JSMN_STRING);
				for(size_t z = 0; z < row.len; ++z) {
					struct column column        = table.columns.items[z];
					struct row_value *row_value = &row.items[z];
					row_value->type             = column.type;
					switch(column.type) {
					case COLUMN_TYPE_ID: {
						row_value->string = json_str8_cpy_push(json, child_value, scratch);
					} break;
					case COLUMN_TYPE_BITMASK: {
						row_value->u32 = 1 << (j + 1);
					} break;
					case COLUMN_TYPE_LABEL: {
						row_value->string = json_str8_cpy_push(json, child_value, scratch);
					} break;
					default: {
					} break;
					}
				}
				i += child_value->size;
				table.rows.items[table.rows.len++] = row;
			}
			i += value->size + 1;
		} else {
			sys_printf("Unexpected key[%d]: %.*s", i, t[i].end - t[i].start, json.str + t[i].start);
		}
	}

	u8 *mem    = NULL;
	usize size = MMEGABYTE(1);
	sys_alloc(mem, size);
	struct marena marena = {0};
	struct alloc alloc   = marena_allocator(&marena);
	marena_init(&marena, mem, size);
	struct str8_list content = {0};
	str8_list_pushf(alloc, &content, "#pragma once\n\n");
	str8_list_pushf(alloc, &content, "#include \"sys-types.h\"\n\n");

	{

		for(size_t i = 0; i < table.columns.len; ++i) {
			struct column column = table.columns.items[i];

			switch(column.type) {
			case COLUMN_TYPE_ID: {
				str8_list_pushf(alloc, &content, "enum %.*s {\n", (i32)table.name.size, table.name.str);
				str8_list_pushf(alloc, &content, "  %.*sNONE = 0,\n", (i32)table.prefix.size, table.prefix.str);
				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row             = table.rows.items[j];
					struct row_value row_value = row.items[i];
					assert(row_value.type == column.type);
					str8_list_pushf(alloc, &content, "  %.*s%.*s,\n", (i32)table.prefix.size, table.prefix.str, (i32)row_value.string.size, row_value.string.str);
				}

				str8_list_pushf(alloc, &content, "};\n\n");
				str8_list_pushf(alloc, &content, "#define %.*s_count %d\n", (i32)table.name.size, table.name.str, (i32)table.rows.len + 1);
				str8_list_pushf(alloc, &content, "\n");

			} break;
			case COLUMN_TYPE_LABEL: {
				str8_list_pushf(alloc, &content, "static char* %.*sLABELS[%d] = {\n", (i32)table.prefix.size, table.prefix.str, (i32)table.rows.len + 1);
				str8_list_pushf(alloc, &content, "  [%.*sNONE] = \"NONE\",\n", (i32)table.prefix.size, table.prefix.str);

				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row                = table.rows.items[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					assert(row_value.type == column.type);
					str8_list_pushf(alloc, &content, "  [%.*s%.*s] = \"%.*s\",\n", (i32)table.prefix.size, table.prefix.str, (i32)row_value_id.string.size, row_value_id.string.str, (i32)row_value.string.size, row_value.string.str);
				}
				str8_list_pushf(alloc, &content, "};\n\n");
			} break;
			case COLUMN_TYPE_BITMASK: {
				str8_list_pushf(alloc, &content, "static u32 %.*sBITMASKS[%d] = {\n", (i32)table.prefix.size, table.prefix.str, (i32)table.rows.len + 1);
				str8_list_pushf(alloc, &content, "  [%.*sNONE] = %d,\n", (i32)table.prefix.size, table.prefix.str, 0);

				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row                = table.rows.items[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					assert(row_value.type == column.type);
					str8_list_pushf(alloc, &content, "  [%.*s%.*s] = %d,\n", (i32)table.prefix.size, table.prefix.str, (i32)row_value_id.string.size, row_value_id.string.str, row_value.u32);
				}
				str8_list_pushf(alloc, &content, "};\n\n");
			} break;
			default: {
			} break;
			}
		}
	}

	// TODO: List concat inplace
	str8 res = {0};

	write_full_file(out_file_path, res.size, res.str);

	return EXIT_SUCCESS;
}

void
gen_tables_recursive(const str8 in_dir)
{
	usize scratch_mem_size = MKILOBYTE(1);
	u8 *scratch_mem_buffer = sys_alloc(NULL, scratch_mem_size);
	assert(scratch_mem_buffer != NULL);
	struct marena scratch_marena = {0};
	marena_init(&scratch_marena, scratch_mem_buffer, scratch_mem_size);
	struct alloc scratch = marena_allocator(&scratch_marena);

	tinydir_dir dir;
	if(tinydir_open(&dir, (char *)in_dir.str) == -1) {
		log_error("Meta", "Cannot open directory: %s", in_dir.str);
		return;
	}

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		str8 in_path   = str8_fmt_push(scratch, "%s/%s", in_dir.str, file.name);
		str8 file_name = str8_cstr(file.name);

		if(file.is_dir) {
			if(
				!str8_match(file_name, str8_lit("."), 0) &&
				!str8_match(file_name, str8_lit(".."), 0)) {
				gen_tables_recursive(in_path);
			}
		} else if(str8_find_needle(file_name, 0, str8_lit(TABLE_EXT), 0)) {
			gen_table(in_path, scratch);
		}
	}
	tinydir_close(&dir);
	sys_free(scratch_mem_buffer);
}

i32
main(i32 argc, char *argv[])
{
	if(argc != 2) {
		printf("Usage: %s <in_path>\n", argv[0]);
		return 1;
	}
	fprintf(stderr, "Processing tables ...\n");
	mkdir(argv[2], 0755);

	gen_tables_recursive(str8_cstr(argv[1]));
	return EXIT_SUCCESS;
}
