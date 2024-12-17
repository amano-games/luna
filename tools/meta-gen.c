#include <tinydir.h>
#include <jsmn.h>

#include "arr.h"
#include "json.h"
#include "sys-cli.c"
#include "mem-arena.c"
#include "mem.c"

#include "str.h"
#include "str.c"
#include "path.c"

#include "mem-arena.h"
#include "sys-io.h"
#include "sys-log.h"
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

struct row_value {
	enum COLUMN_TYPE type;
	union {
		uint32_t u32;
		str8 string;
	};
};

struct row {
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
	struct column *columns;
	struct row *rows;
};

i32
gen_table(const str8 in_path, struct alloc scratch)
{
	log_info("Meta", "Generating table from: %s", in_path.str);

	str8 json = {0};
	json_load(in_path, scratch, &json);
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count   = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
	jsmn_init(&parser);
	i32 json_res = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	assert(json_res == token_count);

	struct table table = {0};
	/* Loop over all keys of the root object */
	for(i32 i = 1; i < token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];

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

			table.columns = arr_ini(value->size, sizeof(*table.columns), scratch);
			for(i32 j = 0; j < value->size; j++) {
				jsmntok_t *child_key   = &tokens[i + j + 1];
				jsmntok_t *child_value = &tokens[i + j + 2];
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
				arr_push(table.columns, column);
			}
			i += value->size + 1;
		} else if(json_eq(json, key, str8_lit("elements")) == 0) {
			assert(value->type == JSMN_ARRAY);
			table.rows = arr_ini(value->size, sizeof(*table.rows), scratch);

			for(i32 j = 0; j < value->size; j++) {
				struct row row            = {0};
				usize count               = arr_len(table.columns);
				row.items                 = arr_ini(count, sizeof(*row.items), scratch);
				struct arr_header *header = arr_header(row.items);
				header->len               = count;

				jsmntok_t *child_key   = &tokens[i + j + 1];
				jsmntok_t *child_value = &tokens[i + j + 2];
				size_t len             = child_value->end - child_value->start;
				assert(child_value->type == JSMN_STRING);
				for(size_t z = 0; z < arr_len(row.items); ++z) {
					struct column column        = table.columns[z];
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
				arr_push(table.rows, row);
			}
			i += value->size + 1;
		} else {
			sys_printf("Unexpected key[%d]: %.*s", i, tokens[i].end - tokens[i].start, json.str + tokens[i].start);
		}
	}

	usize size           = MMEGABYTE(1);
	u8 *mem              = sys_alloc(NULL, size);
	struct marena marena = {0};
	struct alloc alloc   = marena_allocator(&marena);
	marena_init(&marena, mem, size);
	struct str8_list content = {0};
	str8_list_pushf(alloc, &content, "#pragma once\n\n");
	str8_list_pushf(alloc, &content, "#include \"sys-types.h\"\n\n");

	{

		str8 enum_none  = str8_fmt_push(alloc, "%.*sNONE = 0", (i32)table.prefix.size, table.prefix.str);
		str8 enum_count = str8_fmt_push(alloc, "%.*sNUM_COUNT", (i32)table.prefix.size, table.prefix.str);

		for(size_t i = 0; i < arr_len(table.columns); ++i) {
			struct column column = table.columns[i];

			switch(column.type) {
			case COLUMN_TYPE_ID: {
				str8_list_pushf(alloc, &content, "enum %.*s {\n", (i32)table.name.size, table.name.str);
				str8_list_pushf(alloc, &content, "  %.*s,\n\n", (i32)enum_none.size, enum_none.str);
				for(size_t j = 0; j < arr_len(table.rows); ++j) {
					struct row row             = table.rows[j];
					struct row_value row_value = row.items[i];
					assert(row_value.type == column.type);
					str8_list_pushf(alloc, &content, "  %.*s%.*s,\n", (i32)table.prefix.size, table.prefix.str, (i32)row_value.string.size, row_value.string.str);
				}
				str8_list_pushf(alloc, &content, "\n  %.*s,\n", (i32)enum_count.size, enum_count.str);

				str8_list_pushf(alloc, &content, "};\n\n");
				str8_list_pushf(alloc, &content, "\n");

			} break;
			case COLUMN_TYPE_LABEL: {
				str8_list_pushf(alloc, &content, "static char* %.*sLABELS[%.*s] = {\n", (i32)table.prefix.size, table.prefix.str, (i32)enum_count.size, enum_count.str);
				str8_list_pushf(alloc, &content, "  [%.*sNONE] = \"NONE\",\n", (i32)table.prefix.size, table.prefix.str);

				for(size_t j = 0; j < arr_len(table.rows); ++j) {
					struct row row                = table.rows[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					assert(row_value.type == column.type);
					str8_list_pushf(alloc, &content, "  [%.*s%.*s] = \"%.*s\",\n", (i32)table.prefix.size, table.prefix.str, (i32)row_value_id.string.size, row_value_id.string.str, (i32)row_value.string.size, row_value.string.str);
				}
				str8_list_pushf(alloc, &content, "};\n\n");
			} break;
			case COLUMN_TYPE_BITMASK: {
				str8_list_pushf(alloc, &content, "static u32 %.*sBITMASKS[%d] = {\n", (i32)table.prefix.size, table.prefix.str, (i32)arr_len(table.rows) + 1);
				str8_list_pushf(alloc, &content, "  [%.*sNONE] = %d,\n", (i32)table.prefix.size, table.prefix.str, 0);

				for(size_t j = 0; j < arr_len(table.rows); ++j) {
					struct row row                = table.rows[j];
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

	str8 out_file_path = make_file_name_with_ext(scratch, in_path, str8_lit("h"));
	str8 res           = str8_list_join(alloc, &content, NULL);
	void *f            = sys_file_open_w(out_file_path);
	sys_file_w(f, res.str, res.size);
	sys_file_close(f);
	sys_free(mem);

	sys_printf("%s -> %s", in_path.str, out_file_path.str);
	return EXIT_SUCCESS;
}

void
gen_tables_recursive(const str8 in_dir, struct alloc scratch)
{
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
				gen_tables_recursive(in_path, scratch);
			}
		} else if(str8_ends_with(file_name, str8_lit(TABLE_EXT), 0)) {
			gen_table(in_path, scratch);
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);
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

	usize scratch_mem_size = MMEGABYTE(1);
	u8 *scratch_mem_buffer = sys_alloc(NULL, scratch_mem_size);
	assert(scratch_mem_buffer != NULL);
	struct marena scratch_marena = {0};
	marena_init(&scratch_marena, scratch_mem_buffer, scratch_mem_size);
	struct alloc scratch = marena_allocator(&scratch_marena);

	gen_tables_recursive(str8_cstr(argv[1]), scratch);

	sys_free(scratch_mem_buffer);

	return EXIT_SUCCESS;
}
