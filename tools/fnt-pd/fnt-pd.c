#include "fnt-pd.h"
#include "base/arr.h"
#include "base/mem.h"
#include "lib/fnt/fnt.h"
#include "lib/json.h"
#include "base/marena.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "sys/sys-log.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys.h"
#include "base/str.h"

#include <jsmn.h>
#include <tinydir.h>

struct fnt_metrics fnt_pd_handle_metrics(str8 json, struct alloc scratch);
void fnt_pd_handle_line(str8 line, struct fnt *fnt);
b32 pd_fnt_handle_key_value(str8 line, str8 key, str8 *value);
void pd_fnt_handle_file(struct alloc scratch, struct sys_full_file_res data, struct fnt *fnt);

v2 fnt_pd_cell_size_from_tex(str8 in_path, struct alloc scratch);

int
fnt_pd_handle(str8 in_path, str8 out_path, struct alloc scratch)
{
	usize mem_size = MKILOBYTE(100);
	u8 *mem        = mem_alloc_size(scratch, mem_size);
	if(mem == NULL) {
		log_error("fnt-gen", "no memory for %.*s", str8_spread(in_path));
		return -1;
	}

	struct marena marena = {0};
	marena_ini_local(&marena, mem, mem_size);

	struct alloc alloc              = marena_allocator(&marena);
	struct fnt fnt                  = {0};
	fnt.widths                      = arr_new_clr(alloc, fnt.widths, FNT_CHAR_MAX);
	fnt.kern_pairs                  = arr_new_clr(alloc, fnt.kern_pairs, FNT_KERN_PAIRS_MAX);
	arr_header(fnt.widths)->len     = arr_cap(fnt.widths);
	arr_header(fnt.kern_pairs)->len = arr_cap(fnt.kern_pairs);

	struct marena_tmp tmp = marena_tmp_start(&marena);
	v2 cell_size          = fnt_pd_cell_size_from_tex(in_path, alloc);
	fnt.cell_w            = cell_size.x;
	fnt.cell_h            = cell_size.y;
	marena_tmp_end(tmp);

	struct sys_full_file_res res = sys_load_full_file(scratch, in_path);
	pd_fnt_handle_file(scratch, res, &fnt);

	sys_file out_file  = sys_file_zero();
	str8 out_file_path = out_path;
	out_file           = sys_file_open_w(out_file_path);

	if(!sys_file_is_valid(out_file)) {
		log_error("fnt-gen", "can't open file %.*s for writing!", str8_spread(out_file_path));
		return -1;
	}

	struct ser_writer w = {.f = out_file};

	fnt_write(fnt, &w);
	sys_file_close(out_file);

	log_info("fnt-gen", "%.*s -> %.*s", str8_spread(in_path), str8_spread(out_file_path));

	return 1;
}

void
pd_fnt_handle_file(struct alloc scratch, struct sys_full_file_res data, struct fnt *fnt)
{
	str8 bom = str8_lit("\xEF\xBB\xBF");
	str8 str = {.str = data.data, .size = data.size};

	if(str8_starts_with(str, bom, 0)) { str = str8_skip(str, bom.size); }

	str8 metrics_key = str8_lit("--metrics=");
	str8 comment_key = str8_lit("--");
	str8 value       = {0};

	struct str8_list list = str8_split_by_string_chars(scratch, str, str8_lit("\n"), 0);

	for(struct str8_node *n = list.first; n != 0; n = n->next) {
		str8 line = str8_skip_chop_whitespace(n->str);

		if(line.size == 0) { continue; }

		if(str8_starts_with(line, metrics_key, 0)) {
			fnt->metrics = fnt_pd_handle_metrics(str8_skip(line, metrics_key.size), scratch);
		} else if(str8_starts_with(line, comment_key, 0)) {
			str8 rest          = str8_skip(line, comment_key.size);
			str8 kern          = str8_skip_chop_whitespace(rest);
			b32 is_white_space = rest.size > 0 && char_is_space(rest.str[0]);

			// It's not a comment its a kerning value
			if(is_white_space && str8_is_integer_signed(kern, 10)) {
				fnt->kern_pairs[((u16)'-' << 8) | '-'] = str8_to_i32(kern);
			}
		} else if(pd_fnt_handle_key_value(line, str8_lit("tracking"), &value)) {
			fnt->tracking = str8_to_i32(value);
		} else if(pd_fnt_handle_key_value(line, str8_lit("width"), &value)) {
			if(fnt->cell_w == 0) {
				fnt->cell_w = str8_to_i32(value);
			}
		} else if(pd_fnt_handle_key_value(line, str8_lit("height"), &value)) {
			if(fnt->cell_h == 0) {
				fnt->cell_h = str8_to_i32(value);
			}
		} else if(pd_fnt_handle_key_value(line, str8_lit("data"), &value)) {
			log_warn("fnt-pd", "Standalone fonts with embedded data are not supported yet");
		} else if(pd_fnt_handle_key_value(line, str8_lit("datalen"), &value)) {
		} else {
			fnt_pd_handle_line(line, fnt);
		}
	}
}

struct fnt_metrics
fnt_pd_handle_metrics(str8 json, struct alloc scratch)
{
	jsmn_parser parser;
	jsmn_init(&parser);
	struct fnt_metrics res = {0};
	i32 token_count        = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);

	if(token_count <= 0) {
		log_warn("fnt-pd", "Invalid metrics: %.*s", str8_spread(json));
		return res;
	}

	jsmn_init(&parser);
	jsmntok_t *tokens = arr_new(scratch, tokens, token_count);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);

	if(json_res != token_count || tokens[0].type != JSMN_OBJECT) {
		log_warn("fnt-pd", "Invalid metrics: %.*s", str8_spread(json));
		return res;
	}

	ssize i = 1;
	for(i32 p = 0; p < tokens[0].size && i + 1 < token_count; ++p) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(value->type == JSMN_PRIMITIVE) {
			if(json_eq(json, key, str8_lit("baseline")) == 0) {
				res.baseline = json_parse_i32(json, value);
			} else if(json_eq(json, key, str8_lit("xHeight")) == 0) {
				res.x_height = json_parse_i32(json, value);
			} else if(json_eq(json, key, str8_lit("capHeight")) == 0) {
				res.cap_height = json_parse_i32(json, value);
			} else if(json_eq(json, key, str8_lit("descent")) == 0) {
				res.descent = json_parse_i32(json, value);
			}
		}
		i += 1 + json_skip(tokens, i + 1);
	}
	return res;
}

void
fnt_pd_handle_line(str8 line, struct fnt *fnt)
{
	str8 space_key = str8_lit("space");

	if(str8_starts_with(line, space_key, 0)) {
		str8 value           = str8_skip_chop_whitespace(str8_substr(line, (union rng_u64){.min = space_key.size, .max = line.size}));
		fnt->widths[(u8)' '] = str8_to_i32(value);
	} else {
		if(char_is_ascii(line.str[0]) && char_is_space(line.str[1])) {
			str8 value     = str8_skip_chop_whitespace(str8_substr(line, (union rng_u64){.min = 1, .max = line.size}));
			u16 i          = line.str[0];
			fnt->widths[i] = str8_to_i32(value);
		} else if(char_is_ascii(line.str[0]) && char_is_ascii(line.str[1])) {
			str8 value         = str8_skip_chop_whitespace(str8_substr(line, (union rng_u64){.min = 2, .max = line.size}));
			u8 a               = line.str[0];
			u8 b               = line.str[1];
			u16 i              = ((u16)a << 8) | b;
			fnt->kern_pairs[i] = str8_to_i32(value);
		}
	}
}

v2
fnt_pd_cell_size_from_tex(str8 in_path, struct alloc scratch)
{
	str8 fnt_file_name = str8_cpy_push(scratch, str8_skip_last_slash(str8_chop_last_dot(in_path)));
	str8 dir_path      = str8_cpy_push(scratch, str8_chop_last_slash(in_path));
	str8 table_id      = str8_lit("-table-");
	tinydir_dir *dir   = alloc_struct(scratch, dir);
	tinydir_open(dir, (char *)dir_path.str);
	v2 res = {0};

	while(dir->has_next) {
		tinydir_file file;
		tinydir_readfile(dir, &file);
		if(!file.is_dir) {
			str8 file_path = str8_cstr(file.path);
			str8 file_name = str8_cstr(file.name);
			str8 start     = fnt_file_name;
			if(!str8_match(file_path, in_path, 0)) {
				if(str8_starts_with(file_name, start, 0)) {
					usize i = str8_find_needle(file_name, 0, table_id, 0);
					// Make sure that the table id is right after the name
					if(i == start.size) {
						str8 info    = str8_chop_last_dot(str8_substr(file_name, (union rng_u64){.min = i + table_id.size, .max = file_name.size}));
						usize dash_i = str8_find_needle(info, 0, str8_lit("-"), 0);
						str8 w       = str8_substr(info, (union rng_u64){.min = 0, .max = dash_i});
						str8 h       = str8_substr(info, (union rng_u64){.min = dash_i + 1, .max = info.size});
						res.x        = str8_to_i32(w);
						res.y        = str8_to_i32(h);
						break;
					}
				}
			}
		}
		tinydir_next(dir);
	}

	tinydir_close(dir);

	return res;
}

b32
pd_fnt_handle_key_value(str8 line, str8 key, str8 *value)
{
	if(!str8_starts_with(line, key, 0)) { return false; }

	str8 rest = str8_skip_chop_whitespace(str8_skip(line, key.size));

	if(rest.size == 0 || rest.str[0] != '=') { return false; }

	*value = str8_skip_chop_whitespace(str8_skip(rest, 1));

	return true;
}
