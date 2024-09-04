#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../external/jsmn.h"
#include "io.h"
#include "utils.h"

#define TABLE_EXT ".luntable"

struct string {
	size_t len;
	char *text;
};

enum COLUMN_TYPE {
	COLUMN_TYPE_NONE,
	COLUMN_TYPE_ID,
	COLUMN_TYPE_LABEL,
	COLUMN_TYPE_BITMASK,
};

struct column {
	struct string name;
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
		struct string string;
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
	struct string name;
	struct string prefix;
	struct columns columns;
	struct rows rows;
};

static int
jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	if(tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int
gen_table(const char *in_path)
{
	char file_h_path[FILENAME_MAX];
	char file_basename[FILENAME_MAX];
	change_extension(in_path, file_h_path, "h");
	get_basename(in_path, file_basename);

	struct read_file_result res = read_file(in_path);
	char *json                  = res.contents;
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 JSON tokens */
	jsmn_init(&p);
	int r = jsmn_parse(&p, json, strlen(json), t, 128); // "s" is the char array holding the json content
	if(r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return EXIT_FAILURE;
	}

	printf("tokens: %d \n", r);
	printf("%s -> %s \n\n", in_path, file_h_path);

	struct table table = {0};
	/* Loop over all keys of the root object */
	for(int i = 1; i < r; i++) {
		jsmntok_t *key   = &t[i];
		jsmntok_t *value = &t[i + 1];

		if(jsoneq(json, key, "name") == 0) {
			assert(value->type == JSMN_STRING);
			size_t len      = value->end - value->start;
			table.name.len  = len;
			table.name.text = strndup(json + value->start, len);
			i++;
		} else if(jsoneq(json, key, "prefix") == 0) {
			assert(value->type == JSMN_STRING);
			size_t len        = value->end - value->start;
			table.prefix.len  = len;
			table.prefix.text = strndup(json + value->start, len);
			i++;
		} else if(jsoneq(json, key, "columns") == 0) {
			assert(value->type == JSMN_ARRAY);

			table.columns.items = malloc(sizeof(struct column) * value->size);
			table.columns.cap   = value->size;
			for(int j = 0; j < value->size; j++) {
				jsmntok_t *child_key   = &t[i + j + 1];
				jsmntok_t *child_value = &t[i + j + 2];
				size_t len             = child_value->end - child_value->start;

				struct column column = {0};
				column.name.len      = len;
				column.name.text     = strndup(json + child_value->start, len);
				if(jsoneq(json, child_value, "id") == 0) {
					column.type = COLUMN_TYPE_ID;
				} else if(jsoneq(json, child_value, "label") == 0) {
					column.type = COLUMN_TYPE_LABEL;
				} else if(jsoneq(json, child_value, "bitmask") == 0) {
					column.type = COLUMN_TYPE_BITMASK;
				} else {
					assert(0);
				}
				table.columns.items[table.columns.len++] = column;
			}
			i += value->size + 1;
		} else if(jsoneq(json, key, "elements") == 0) {
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
						row_value->string      = (struct string){.len = len};
						row_value->string.text = strndup(json + child_value->start, len);
					} break;
					case COLUMN_TYPE_BITMASK: {
						row_value->u32 = 1 << (j + 1);
					} break;
					case COLUMN_TYPE_LABEL: {
						row_value->string      = (struct string){.len = len};
						row_value->string.text = strndup(json + child_value->start, len);
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
			printf("Unexpected key[%d]: %.*s\n", i, t[i].end - t[i].start, json + t[i].start);
		}
	}

	struct string file_h_content = {0};
	file_h_content.text          = malloc(MMEGABYTE(1));

	{
		struct string *content = &file_h_content;
		char *buffer           = file_h_content.text;
		content->len += sprintf(buffer + content->len, "#pragma once\n\n");
		content->len += sprintf(buffer + content->len, "#include \"sys-types.h\"\n\n");
	}

	{

		for(size_t i = 0; i < table.columns.len; ++i) {
			struct column column = table.columns.items[i];

			switch(column.type) {
			case COLUMN_TYPE_ID: {
				struct string *content = &file_h_content;
				char *buffer           = file_h_content.text;
				content->len += sprintf(buffer + content->len, "enum %.*s {\n", (int)table.name.len, table.name.text);
				content->len += sprintf(buffer + content->len, "  %.*sNONE = 0,\n", (int)table.prefix.len, table.prefix.text);
				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row             = table.rows.items[j];
					struct row_value row_value = row.items[i];
					assert(row_value.type == column.type);
					content->len += sprintf(
						buffer + content->len,
						"  %.*s%.*s,\n",
						(int)table.prefix.len,
						table.prefix.text,
						(int)row_value.string.len,
						row_value.string.text);
				}

				content->len += sprintf(buffer + content->len, "};\n\n");
				content->len += sprintf(buffer + content->len, "#define %.*s_count %d\n", (int)table.name.len, table.name.text, (int)table.rows.len + 1);
				content->len += sprintf(buffer + content->len, "\n");

			} break;
			case COLUMN_TYPE_LABEL: {
				struct string *content = &file_h_content;
				char *buffer           = file_h_content.text;
				content->len += sprintf(
					buffer + content->len,
					"static char* %.*sLABELS[%d] = {\n",
					(int)table.prefix.len,
					table.prefix.text,
					(int)table.rows.len + 1);
				content->len += sprintf(buffer + content->len, "  [%.*sNONE] = \"NONE\",\n", (int)table.prefix.len, table.prefix.text);

				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row                = table.rows.items[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					assert(row_value.type == column.type);
					content->len += sprintf(
						buffer + content->len,
						"  [%.*s%.*s] = \"%.*s\",\n",
						(int)table.prefix.len,
						table.prefix.text,
						(int)row_value_id.string.len,
						row_value_id.string.text,
						(int)row_value.string.len,
						row_value.string.text);
				}
				content->len += sprintf(buffer + content->len, "};\n\n");
			} break;
			case COLUMN_TYPE_BITMASK: {
				struct string *content = &file_h_content;
				char *buffer           = file_h_content.text;
				content->len += sprintf(
					buffer + content->len,
					"static u32 %.*sBITMASKS[%d] = {\n",
					(int)table.prefix.len,
					table.prefix.text,
					(int)table.rows.len + 1);
				content->len += sprintf(buffer + content->len, "  [%.*sNONE] = %d,\n", (int)table.prefix.len, table.prefix.text, 0);

				for(size_t j = 0; j < table.rows.len; ++j) {
					struct row row                = table.rows.items[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					assert(row_value.type == column.type);
					content->len += sprintf(
						buffer + content->len,
						"  [%.*s%.*s] = %d,\n",
						(int)table.prefix.len,
						table.prefix.text,
						(int)row_value_id.string.len,
						row_value_id.string.text,
						row_value.u32);
				}
				content->len += sprintf(buffer + content->len, "};\n\n");

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
				// 	char *buffer           = file_c_content.text;
				// 	content->len += sprintf(
				// 		buffer + content->len,
				// 		"char*\n%.*s_label(enum %.*s value)\n{\n  return LABELS[value];\n}\n\n",
				// 		(int)table.name.len,
				// 		table.name.text,
				// 		(int)table.name.len,
				// 		table.name.text);
				// }
			} break;
			case COLUMN_TYPE_BITMASK: {
				// {
				// 	struct string *content = &file_c_content;
				// 	char *buffer           = file_c_content.text;
				// 	content->len += sprintf(
				// 		buffer + content->len,
				// 		"inline u32\n%.*s_mask(enum %.*s value)\n{\n  return BITMASKS[value];\n}\n\n",
				// 		(int)table.name.len,
				// 		table.name.text,
				// 		(int)table.name.len,
				// 		table.name.text);
				// }
			} break;
			default: break;
			}
			// u32 component_type_bitmask(enum component_type value);
			// char * component_type_label(enum component_type value);
		}
	}

	write_file(file_h_path, file_h_content.len, file_h_content.text);

	return EXIT_SUCCESS;
}

void
gen_tables_recursive(const char *in_dir)
{
	DIR *dir;
	struct dirent *entry;
	struct stat statbuf;
	char in_path[FILENAME_MAX], out_path[FILENAME_MAX];

	dir = opendir(in_dir);
	if(dir == NULL) {
		fprintf(stderr, "Cannot open directory: %s\n", in_dir);
		return;
	}

	while((entry = readdir(dir)) != NULL) {
		snprintf(in_path, sizeof(in_path), "%s/%s", in_dir, entry->d_name);

		// printf("in loop: %s\n", in_path);
		// printf("out loop: %s\n", out_path);

		if(stat(in_path, &statbuf) == -1) {
			perror("Error getting file information");
			continue;
		}

		if(S_ISDIR(statbuf.st_mode)) {
			if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				gen_tables_recursive(in_path);
			}
		} else {
			if(strstr(entry->d_name, TABLE_EXT) != NULL) {
				gen_table(in_path);
			}
		}
	}
	closedir(dir);
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

	gen_tables_recursive(argv[1]);
	return EXIT_SUCCESS;
}
