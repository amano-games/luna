#include "fnt-pd.h"
#include "arr.h"
#include "assets/fnt.h"
#include "json.h"
#include "mem-arena.h"
#include "str.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-types.h"
#include "sys-utils.h"
#include "sys.h"
#include "sys-assert.h"
#include "str.h"
#include <jsmn.h>
#include <tinydir.h>

#define ASCII_MAX 128

bool32
fnt_pd_load(const str8 path, struct alloc alloc, str8 *out)
{
	void *f = sys_file_open_r(path);
	if(!f) {
		log_warn("fnt-pd", "Can't open %s\n", path.str);
		return 0;
	}
	sys_file_seek_end(f, 0);
	i32 size = sys_file_tell(f);
	sys_file_seek_set(f, 0);
	u8 *buf = (u8 *)alloc.allocf(alloc.ctx, (usize)size + 1);
	if(!buf) {
		sys_file_close(f);
		log_error("fnt-pd", "loading %s", path.str);
		return 0;
	}
	sys_file_r(f, buf, size);
	sys_file_close(f);
	buf[size] = '\0';
	out->str  = buf;
	out->size = size;
	return 1;
}

struct fnt_metrics
handle_metrics(str8 json, struct alloc scratch)
{
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	assert(json_res == token_count);

	struct fnt_metrics res = {0};
	for(i32 i = 1; i < token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("baseline")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.baseline = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("xHeight")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.x_height = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("capHeight")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.cap_height = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("descent")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.descent = json_parse_i32(json, value);
		}
	}
	return res;
}

void
handle_line(str8 line, struct fnt *fnt)
{
	str8 space_id = str8_lit("space");
	if(str8_starts_with(line, space_id, 0)) {
		str8 value           = str8_skip_chop_whitespace(str8_substr(line, (union rng_u64){.min = space_id.size, .max = line.size}));
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

void
handle_lines(str8 str, struct fnt *fnt)
{
	str8 string = str;
	u8 *ptr     = string.str;
	u8 *opl     = string.str + string.size;
	for(; ptr < opl;) {
		u8 *first = ptr;
		for(; ptr < opl; ptr += 1) {
			u8 c            = *ptr;
			bool32 is_split = 0;
			if('\n' == c) {
				is_split = 1;
				break;
			}
			if(is_split) {
				break;
			}
		}

		// TODO: skip commented lines
		str8 line = str8_range(first, ptr);
		if(line.size > 0) {
			handle_line(line, fnt);
		}
		ptr += 1;
	}
}

v2
get_fnt_size(str8 in_path, struct alloc scratch)
{
	str8 fnt_file_name = str8_cpy_push(scratch, str8_skip_last_slash(str8_chop_last_dot(in_path)));
	str8 dir_path      = str8_cpy_push(scratch, str8_chop_last_slash(in_path));
	str8 table_id      = str8_lit("-table-");
	tinydir_dir dir    = {0};
	tinydir_open(&dir, (char *)dir_path.str);
	v2 res = {0};

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		if(!file.is_dir) {
			str8 file_path = str8_cstr(file.path);
			str8 file_name = str8_cstr(file.name);
			str8 start     = fnt_file_name;
			if(!str8_match(file_path, in_path, 0)) {
				if(str8_starts_with(file_name, start, 0)) {
					usize i      = str8_find_needle(file_name, 0, table_id, 0);
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
		tinydir_next(&dir);
	}

	tinydir_close(&dir);

	return res;
}

int
handle_fnt_pd(str8 in_path, str8 out_path, struct alloc scratch)
{
	usize mem_size = MKILOBYTE(100);
	u8 *mem_buffer = sys_alloc(NULL, mem_size);
	assert(mem_buffer != NULL);
	struct marena marean = {0};
	marena_init(&marean, mem_buffer, mem_size);
	struct alloc alloc = marena_allocator(&marean);

	str8 data = {0};
	fnt_pd_load(in_path, scratch, &data);

	struct fnt fnt = {0};

	// TODO: Reset scratch
	v2 cell_size = get_fnt_size(in_path, scratch);
	fnt.cell_w   = cell_size.x;
	fnt.cell_h   = cell_size.y;

	fnt.widths                      = arr_ini(FNT_CHAR_MAX, sizeof(*fnt.widths), alloc);
	fnt.kern_pairs                  = arr_ini(FNT_KERN_PAIRS_MAX, sizeof(*fnt.kern_pairs), alloc);
	arr_header(fnt.widths)->len     = arr_cap(fnt.widths);
	arr_header(fnt.kern_pairs)->len = arr_cap(fnt.kern_pairs);

	str8 metrics_id  = str8_lit("--metrics=");
	str8 tracking_id = str8_lit("tracking=");
	usize metrics_i  = str8_find_needle(data, 0, metrics_id, 0);
	assert((char)*(data.str + metrics_i + metrics_id.size) == '{');
	usize metrics_f  = str8_find_needle(data, metrics_i, str8_lit("}"), 0);
	usize tracking_i = str8_find_needle(data, 0, tracking_id, 0);
	usize tracking_f = str8_find_needle(data, tracking_i, str8_lit("\n"), 0);

	str8 metrics_str  = {.str = data.str + metrics_i, .size = metrics_f - metrics_i};
	str8 tracking_str = {.str = data.str + tracking_i, .size = tracking_f - tracking_i};

	str8 metrics_val  = str8_postfix(metrics_str, metrics_str.size - metrics_id.size);
	str8 tracking_val = str8_postfix(tracking_str, tracking_str.size - tracking_id.size);

	// The value is not null terminated, therefore we need to increase the size
	metrics_val.size++;
	tracking_val.size++;

	fnt.metrics  = handle_metrics(metrics_val, scratch);
	fnt.tracking = str8_to_i32(tracking_val);

	str8 rest = str8_skip_chop_whitespace((str8){
		.str  = data.str + tracking_f,
		.size = data.size - tracking_f,
	});

	handle_lines(rest, &fnt);

	void *out_file;
	str8 out_file_path = out_path;
	if(!(out_file = sys_file_open_w(out_file_path))) {
		log_error("ai-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}
	// TODO: Serialize font
	struct ser_writer w = {
		.f = out_file,
	};

	fnt_write(fnt, &w);
	sys_file_close(out_file);

	sys_free(mem_buffer);
	log_info("fnt-gen", "%s -> %s\n", in_path.str, out_file_path.str);

	return 1;
}
