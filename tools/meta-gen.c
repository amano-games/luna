#include <tinydir.h>
#include <jsmn.h>
#include "base/dbg.h"
#include "whereami.c"

#include "sys/sys.h"
#include "sys/sys-cli.c"
#include "sys/sys-io.h"

#include "base/arr.h"
#include "base/marena.c"
#include "base/mem.c"
#include "base/str.h"
#include "base/str.c"
#include "base/path.c"
#include "base/marena.h"
#include "base/log.h"
#include "base/utils.h"

#include "lib/json.h"

#define TABLE_EXT ".luntable"
#define LOG_ID    "meta-gen"

enum COLUMN_TYPE {
	COLUMN_TYPE_NONE,

	COLUMN_TYPE_ID,
	COLUMN_TYPE_LABEL,
	COLUMN_TYPE_BITMASK,

	COLUMN_TYPE_NUM_COUNT,
};

struct column {
	str8 name;
	enum COLUMN_TYPE type;
};

struct row_value {
	enum COLUMN_TYPE type;
	union {
		u8 u8;
		u16 u16;
		u32 u32;
		u64 u64;
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
	str8 json          = {0};
	b32 has_bitmasks   = false;
	b32 has_labels     = false;
	struct table table = {0};
	jsmn_parser parser;

	json_load(in_path, scratch, &json);

	jsmn_init(&parser);
	i32 token_count   = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmntok_t *tokens = arr_new(tokens, token_count, scratch);
	jsmn_init(&parser);

	i32 json_res = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	dbg_assert(json_res == token_count);

	/* Loop over all keys of the root object */
	for(i32 i = 1; i < token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];

		if(json_eq(json, key, str8_lit("name")) == 0) {
			dbg_assert(value->type == JSMN_STRING);
			table.name = json_str8_cpy_push(json, value, scratch);
			i++;
		} else if(json_eq(json, key, str8_lit("prefix")) == 0) {
			dbg_assert(value->type == JSMN_STRING);
			table.prefix = json_str8_cpy_push(json, value, scratch);
			i++;
		} else if(json_eq(json, key, str8_lit("columns")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);

			table.columns = arr_new(table.columns, value->size, scratch);
			for(i32 j = 0; j < value->size; j++) {
				jsmntok_t *child_key   = &tokens[i + j + 1];
				jsmntok_t *child_value = &tokens[i + j + 2];
				size len               = child_value->end - child_value->start;

				struct column column = {0};
				column.name          = json_str8_cpy_push(json, child_value, scratch);
				if(json_eq(json, child_value, str8_lit("id")) == 0) {
					column.type = COLUMN_TYPE_ID;
				} else if(json_eq(json, child_value, str8_lit("label")) == 0) {
					column.type = COLUMN_TYPE_LABEL;
					has_labels  = true;
				} else if(json_eq(json, child_value, str8_lit("bitmask")) == 0) {
					has_bitmasks = true;
					column.type  = COLUMN_TYPE_BITMASK;
				} else {
					dbg_assert(0);
				}
				arr_push(table.columns, column);
			}
			i += value->size + 1;
		} else if(json_eq(json, key, str8_lit("elements")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			size rows_count = value->size;
			table.rows      = arr_new(table.rows, rows_count, scratch);

			for(size j = 0; j < rows_count; j++) {
				struct row row            = {0};
				usize count               = arr_len(table.columns);
				row.items                 = arr_new(row.items, count, scratch);
				struct arr_header *header = arr_header(row.items);
				header->len               = count;

				jsmntok_t *child_key   = &tokens[i + j + 1];
				jsmntok_t *child_value = &tokens[i + j + 2];
				size_t len             = child_value->end - child_value->start;
				dbg_assert(child_value->type == JSMN_STRING);
				for(size_t z = 0; z < arr_len(row.items); ++z) {
					struct column column        = table.columns[z];
					struct row_value *row_value = &row.items[z];
					row_value->type             = column.type;
					switch(column.type) {
					case COLUMN_TYPE_ID: {
						row_value->string = json_str8_cpy_push(json, child_value, scratch);
					} break;
					case COLUMN_TYPE_BITMASK: {
						u8 flag = j + 1;
						if(rows_count < 8) {
							row_value->u8 = (u8)1 << (u8)flag;
						} else if(rows_count < 16) {
							row_value->u16 = (u16)1 << (u16)flag;
						} else if(rows_count < 32) {
							row_value->u32 = (u32)1 << (u32)flag;
						} else {
							row_value->u64 = (u64)1 << (u64)flag;
						}
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
			log_warn(LOG_ID, "Unexpected key[%d]: %.*s", i, tokens[i].end - tokens[i].start, json.str + tokens[i].start);
		}
	}

	usize size           = MMEGABYTE(1);
	u8 *mem              = sys_alloc(NULL, size);
	struct marena marena = {0};
	struct alloc alloc   = marena_allocator(&marena);
	marena_init(&marena, mem, size);
	struct str8_list content = {0};
	str8_list_pushf(alloc, &content, "#pragma once\n\n");
	str8_list_pushf(alloc, &content, "/* THIS FILE IS AUTOGENERATED */\n\n");
	if(has_labels) {
		str8_list_pushf(alloc, &content, "#include \"base/str.h\"\n");
	}
	if(has_bitmasks) {
		str8_list_pushf(alloc, &content, "#include \"base/types.h\"\n");
	}

	if(has_labels || has_bitmasks) {
		str8_list_push(alloc, &content, str8_lit("\n"));
	}

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
					dbg_assert(row_value.type == column.type);
					str8_list_pushf(alloc, &content, "  %.*s%.*s,\n", (i32)table.prefix.size, table.prefix.str, (i32)row_value.string.size, row_value.string.str);
				}
				str8_list_pushf(alloc, &content, "\n  %.*s,\n", (i32)enum_count.size, enum_count.str);
				str8_list_pushf(alloc, &content, "};\n\n");
				str8_list_pushf(alloc, &content, "\n");

			} break;
			case COLUMN_TYPE_LABEL: {
				str8_list_pushf(alloc, &content, "static const str8 %.*sLABELS[%.*s] = {\n", (i32)table.prefix.size, table.prefix.str, (i32)enum_count.size, enum_count.str);
				str8_list_pushf(alloc, &content, "  [%.*sNONE] = str8_lit_comp(\"NONE\"),\n", (i32)table.prefix.size, table.prefix.str);

				for(size_t j = 0; j < arr_len(table.rows); ++j) {
					struct row row                = table.rows[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					dbg_assert(row_value.type == column.type);
					str8_list_pushf(
						alloc,
						&content,
						"  [%.*s%.*s] = str8_lit_comp(\"%.*s\"),\n",
						(i32)table.prefix.size,
						table.prefix.str,
						(i32)row_value_id.string.size,
						row_value_id.string.str,
						(i32)row_value.string.size,
						row_value.string.str);
				}
				str8_list_pushf(alloc, &content, "};\n\n");
			} break;
			case COLUMN_TYPE_BITMASK: {
				size_t rows_count = arr_len(table.rows) + 1;
				str8 bitmask_type = {0};
				dbg_assert(rows_count <= 64);
				if(rows_count < 8) {
					bitmask_type = str8_lit("u8");
				} else if(rows_count < 16) {
					bitmask_type = str8_lit("u16");
				} else if(rows_count < 32) {
					bitmask_type = str8_lit("u32");
				} else {
					bitmask_type = str8_lit("u64");
				}
				str8_list_pushf(
					alloc,
					&content,
					"static %.*s %.*sBITMASKS[%d] = {\n",
					(i32)bitmask_type.size,
					bitmask_type.str,
					(i32)table.prefix.size,
					table.prefix.str,
					(i32)arr_len(table.rows) + 1);
				str8_list_pushf(alloc, &content, "  [%.*sNONE] = %d,\n", (i32)table.prefix.size, table.prefix.str, 0);

				for(size_t j = 0; j < arr_len(table.rows); ++j) {
					struct row row                = table.rows[j];
					struct row_value row_value    = row.items[i];
					struct row_value row_value_id = row.items[0];
					dbg_assert(row_value.type == column.type);
					str8_list_pushf(
						alloc,
						&content,
						"  [%.*s%.*s] = %" PRIu64 ",\n",
						(i32)table.prefix.size,
						table.prefix.str,
						(i32)row_value_id.string.size,
						row_value_id.string.str,
						row_value.u64);
				}
				str8_list_pushf(alloc, &content, "};\n\n");
			} break;
			default: {
			} break;
			}
		}
	}

	str8 out_file_path = path_make_file_name_with_ext(scratch, in_path, str8_lit("h"));
	str8 res           = str8_list_join(alloc, &content, NULL);
	void *f            = sys_file_open_w(out_file_path);
	sys_file_w(f, res.str, res.size);
	sys_file_close(f);
	sys_free(mem);

	log_info(LOG_ID, "items:%-2d %s -> %s", (int)arr_len(table.rows), in_path.str + 3, out_file_path.str + 3);
	return EXIT_SUCCESS;
}

void
gen_tables_recursive(const str8 in_dir, struct marena *arena)
{
	tinydir_dir dir;
	struct alloc alloc = marena_allocator(arena);

	if(tinydir_open(&dir, (char *)in_dir.str) == -1) {
		log_error(LOG_ID, "Cannot open directory: %s", in_dir.str);
		return;
	}

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		str8 in_path   = str8_fmt_push(alloc, "%s/%s", in_dir.str, file.name);
		str8 file_name = str8_cstr(file.name);

		if(file.is_dir) {
			if(
				!str8_match(file_name, str8_lit("."), 0) &&
				!str8_match(file_name, str8_lit(".."), 0)) {
				gen_tables_recursive(in_path, arena);
			}
		} else if(str8_ends_with(file_name, str8_lit(TABLE_EXT), 0)) {
			gen_table(in_path, alloc);
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);
}

i32
main(i32 argc, char *argv[])
{
	int res = EXIT_FAILURE;
	if(argc != 2) {
		sys_printf("Usage: %s <in_path>\n", argv[0]);
		res = EXIT_SUCCESS;
		return res;
	}

	str8 in_path        = str8_cstr(argv[1]);
	usize mem_size      = MMEGABYTE(1);
	u8 *mem             = sys_alloc(NULL, mem_size);
	struct marena arena = {0};
	dbg_check_warn(mem, LOG_ID, "Failed to get scratch memory");

	marena_init(&arena, mem, mem_size);
	log_info(LOG_ID, "Processing C from %s -> %s", in_path.str, in_path.str);
	gen_tables_recursive(in_path, &arena);

	res = EXIT_SUCCESS;

error:;
	if(mem) {
		sys_free(mem);
	}

	return res;
}
