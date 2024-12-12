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
#include "str.h"
#include "sys-utils.h"
#include "sys.h"
#include "utils.h"

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

int
gen_table(const str8 in_path, struct alloc scratch)
{
	char file_basename[FILENAME_MAX];
	str8 out_file_path = make_file_name_with_ext(scratch, str8_cstr((char *)in_path.str), str8_lit("h"));
	get_basename((char *)in_path.str, file_basename);

	str8 json = {0};
	json_load(in_path, scratch, &json);
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 JSON tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, (char *)json.str, json.size, t, 128); // "s" is the char array holding the json content
	if(r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return EXIT_FAILURE;
	}

	printf("tokens: %d \n", r);
	printf("%s -> %s \n\n", in_path.str, out_file_path.str);

	struct table table = {0};
	/* Loop over all keys of the root object */
	for(int i = 1; i < r; i++) {
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
			for(int j = 0; j < value->size; j++) {
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

			for(int j = 0; j < value->size; j++) {
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
			printf("Unexpected key[%d]: %.*s\n", i, t[i].end - t[i].start, json.str + t[i].start);
		}
	}

	struct str8 file_h_content = {0};
	file_h_content.str         = malloc(MMEGABYTE(1));

	{
		struct str8 *content = &file_h_content;
		char *buffer         = (char *)file_h_content.str;
		content->size += sprintf(buffer + content->size, "#pragma once\n\n");
		content->size += sprintf(buffer + content->size, "#include \"sys-types.h\"\n\n");
	}

	{

		for(size_t i = 0; i < table.columns.len; ++i) {
			struct column column = table.columns.items[i];

			switch(column.type) {
			case COLUMN_TYPE_ID: {
				struct str8 *content = &file_h_content;
				char *buffer         = (char *)file_h_content.str;
				content->size += sprintf(buffer + content->size, "enum %.*s {\n", (int)table.name.size, table.name.str);
				content->size += sprintf(buffer + content->size, "  %.*sNONE = 0,\n", (int)table.prefix.size, table.prefix.str);
				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row             = table.rows.items[j];
					struct row_value row_value = row.items[i];
					assert(row_value.type == column.type);
					content->size += sprintf(
						buffer + content->size,
						"  %.*s%.*s,\n",
						(int)table.prefix.size,
						table.prefix.str,
						(int)row_value.string.size,
						row_value.string.str);
				}

				content->size += sprintf(buffer + content->size, "};\n\n");
				content->size += sprintf(buffer + content->size, "#define %.*s_count %d\n", (int)table.name.size, table.name.str, (int)table.rows.len + 1);
				content->size += sprintf(buffer + content->size, "\n");

			} break;
			case COLUMN_TYPE_LABEL: {
				str8 *content = &file_h_content;
				char *buffer  = (char *)file_h_content.str;
				content->size += sprintf(
					buffer + content->size,
					"static char* %.*sLABELS[%d] = {\n",
					(int)table.prefix.size,
					table.prefix.str,
					(int)table.rows.len + 1);
				content->size += sprintf(buffer + content->size, "  [%.*sNONE] = \"NONE\",\n", (int)table.prefix.size, table.prefix.str);

				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row                = table.rows.items[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					assert(row_value.type == column.type);
					content->size += sprintf(
						buffer + content->size,
						"  [%.*s%.*s] = \"%.*s\",\n",
						(int)table.prefix.size,
						table.prefix.str,
						(int)row_value_id.string.size,
						row_value_id.string.str,
						(int)row_value.string.size,
						row_value.string.str);
				}
				content->size += sprintf(buffer + content->size, "};\n\n");
			} break;
			case COLUMN_TYPE_BITMASK: {
				str8 *content = &file_h_content;
				char *buffer  = (char *)file_h_content.str;
				content->size += sprintf(
					buffer + content->size,
					"static u32 %.*sBITMASKS[%d] = {\n",
					(int)table.prefix.size,
					table.prefix.str,
					(int)table.rows.len + 1);
				content->size += sprintf(buffer + content->size, "  [%.*sNONE] = %d,\n", (int)table.prefix.size, table.prefix.str, 0);

				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row                = table.rows.items[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					assert(row_value.type == column.type);
					content->size += sprintf(
						buffer + content->size,
						"  [%.*s%.*s] = %d,\n",
						(int)table.prefix.size,
						table.prefix.str,
						(int)row_value_id.string.size,
						row_value_id.string.str,
						row_value.u32);
				}
				content->size += sprintf(buffer + content->size, "};\n\n");

			} break;
			default: {
			} break;
			}
		}
	}

	{
		for(size_t i = 0; i < table.columns.len; ++i) {
			struct column column = table.columns.items[i];
			switch(column.type) {
			case COLUMN_TYPE_LABEL: {
				// {
				// 	struct string *content = &file_c_content;
				// 	char *buffer           = file_c_content.str;
				// 	content->len += sprintf(
				// 		buffer + content->len,
				// 		"char*\n%.*s_label(enum %.*s value)\n{\n  return LABELS[value];\n}\n\n",
				// 		(int)table.name.len,
				// 		table.name.str,
				// 		(int)table.name.len,
				// 		table.name.str);
				// }
			} break;
			case COLUMN_TYPE_BITMASK: {
				// {
				// 	struct string *content = &file_c_content;
				// 	char *buffer           = file_c_content.str;
				// 	content->len += sprintf(
				// 		buffer + content->len,
				// 		"inline u32\n%.*s_mask(enum %.*s value)\n{\n  return BITMASKS[value];\n}\n\n",
				// 		(int)table.name.len,
				// 		table.name.str,
				// 		(int)table.name.len,
				// 		table.name.str);
				// }
			} break;
			default: break;
			}
			// u32 component_type_bitmask(enum component_type value);
			// char * component_type_label(enum component_type value);
		}
	}

	write_file((char *)out_file_path.str, file_h_content.size, file_h_content.str);

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
		fprintf(stderr, "Cannot open directory: %s\n", in_dir.str);
		return;
	}

	char in_path[FILENAME_MAX], out_path[FILENAME_MAX];

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		str8 in_path = str8_fmt_push(scratch, "%s/%s", in_dir.str, file.name);

		if(file.is_dir) {
			if(strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0) {
				gen_tables_recursive(in_path);
			}
		} else {
			if(strstr(file.name, TABLE_EXT) != NULL) {
				gen_table(in_path, scratch);
			}
		}
	}
	tinydir_close(&dir);
	sys_free(scratch_mem_buffer);
}

int
main(int argc, char *argv[])
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
