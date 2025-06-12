#include "fnt.h"

#include "arr.h"
#include "mathfunc.h"
#include "serialize/serialize.h"
#include "str.h"
#include "sys-log.h"
#include "sys-log.h"
#include "dbg.h"

i32
fnt_char_size_x_px(struct fnt fnt, i32 a, i32 b, i32 tracking)
{
	if(a < 32) return 0;
	i32 x            = 0;
	i32 is_last_char = (b == '\n') || (b == -1);
	x += fnt.widths[a] ? fnt.widths[a] : fnt.cell_w;
	u16 kern_i         = ((u16)a << 8) | b;
	i32 has_kern_pairs = (fnt.kern_pairs != NULL) && (b > 0);
	x += has_kern_pairs * (kern_i > 0) * fnt.kern_pairs[kern_i];
	x += (1 - is_last_char) * (tracking + fnt.tracking);

	return x;
}

v2_i32
fnt_size_px(struct fnt fnt, const str8 str, i32 tracking, i32 leading)
{
	i32 x    = 0;
	i32 maxx = 0;
	i32 y    = fnt.cell_h + leading;
	for(usize i = 0; i < str.size; i++) {
		i32 ci         = str.str[i];
		i32 cbi        = (i < str.size - 1) ? str.str[i + 1] : -1;
		i32 is_newline = (ci == '\n');
		i32 move_x     = fnt_char_size_x_px(fnt, ci, cbi, tracking);
		x              = (1 - is_newline) * (x + move_x);
		maxx           = max_i32(maxx, x);
		y += is_newline * (fnt.cell_h + leading);
	}
	return (v2_i32){maxx, y};
}

void
fnt_write(struct fnt fnt, struct ser_writer *w)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("tracking"));
	ser_write_i32(w, fnt.tracking);
	ser_write_string(w, str8_lit("grid_w"));
	ser_write_i32(w, fnt.grid_w);
	ser_write_string(w, str8_lit("grid_h"));
	ser_write_i32(w, fnt.grid_h);
	ser_write_string(w, str8_lit("cell_w"));
	ser_write_i32(w, fnt.cell_w);
	ser_write_string(w, str8_lit("cell_h"));
	ser_write_i32(w, fnt.cell_h);

	ser_write_string(w, str8_lit("metrics"));
	ser_write_object(w);
	ser_write_string(w, str8_lit("baseline"));
	ser_write_i32(w, fnt.metrics.baseline);
	ser_write_string(w, str8_lit("x_height"));
	ser_write_i32(w, fnt.metrics.x_height);
	ser_write_string(w, str8_lit("cap_height"));
	ser_write_i32(w, fnt.metrics.cap_height);
	ser_write_string(w, str8_lit("descent"));
	ser_write_i32(w, fnt.metrics.descent);

	ser_write_end(w);

	ser_write_string(w, str8_lit("widths"));
	ser_write_array(w);
	// TODO: Store same as pairs, only the characters that have the width
	for(usize i = 0; i < arr_len(fnt.widths); ++i) {
		ser_write_u8(w, fnt.widths[i]);
	}
	ser_write_end(w);

	// Kern pairs are stored as an array of objects where the
	// key is the pair characters
	// and the value is the kerning value
	ser_write_string(w, str8_lit("kern_pairs"));
	ser_write_array(w);
	for(usize i = 0; i < arr_len(fnt.kern_pairs); ++i) {
		if(fnt.kern_pairs[i] != 0) {
			ser_write_object(w);
			u16 index    = i;
			u8 a         = (index >> 8) & 0xFF;
			u8 b         = index & 0xFF;
			char pair[3] = {a, b, '\0'};
			ser_write_string(w, str8_cstr(pair));
			ser_write_i32(w, fnt.kern_pairs[i]);
			ser_write_end(w);
		}
	}
	ser_write_end(w);

	ser_write_end(w);
}

i32
fnt_read(struct ser_reader *r, struct fnt *fnt)
{
	struct ser_value obj = ser_read(r);
	struct ser_value key, value;

	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_check(key.type == SER_TYPE_STRING, "fnt", "Corrupt fnt data");
		if(str8_match(key.str, str8_lit("tracking"), 0)) {
			fnt->tracking = value.i32;
		} else if(str8_match(key.str, str8_lit("grid_w"), 0)) {
			fnt->grid_w = value.i32;
		} else if(str8_match(key.str, str8_lit("grid_h"), 0)) {
			fnt->grid_h = value.i32;
		} else if(str8_match(key.str, str8_lit("cell_w"), 0)) {
			fnt->cell_w = value.i32;
		} else if(str8_match(key.str, str8_lit("cell_h"), 0)) {
			fnt->cell_h = value.i32;
		} else if(str8_match(key.str, str8_lit("metrics"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				assert(item_key.type == SER_TYPE_STRING);
				if(str8_match(item_key.str, str8_lit("baseline"), 0)) {
					fnt->metrics.baseline = item_value.i32;
				} else if(str8_match(item_key.str, str8_lit("x_height"), 0)) {
					fnt->metrics.x_height = item_value.i32;
				} else if(str8_match(item_key.str, str8_lit("cap_height"), 0)) {
					fnt->metrics.cap_height = item_value.i32;
				} else if(str8_match(item_key.str, str8_lit("descent"), 0)) {
					fnt->metrics.descent = item_value.i32;
				}
			}
		} else if(str8_match(key.str, str8_lit("widths"), 0)) {
			struct ser_value item_val;
			usize i = 0;
			while(ser_iter_array(r, value, &item_val) && i < arr_cap(fnt->widths)) {
				fnt->widths[i++] = item_val.u8;
			}
		} else if(str8_match(key.str, str8_lit("kern_pairs"), 0)) {
			struct ser_value item_val;
			usize i = 0;
			while(ser_iter_array(r, value, &item_val) && i < arr_cap(fnt->kern_pairs)) {
				assert(item_val.type == SER_TYPE_OBJECT);
				struct ser_value ker_key, ker_value;
				while(ser_iter_object(r, item_val, &ker_key, &ker_value)) {
					assert(ker_key.type == SER_TYPE_STRING);
					assert(ker_value.type == SER_TYPE_I32);
					u8 a                   = ker_key.str.str[0];
					u8 b                   = ker_key.str.str[1];
					u16 index              = ((u16)a << 8) | b;
					fnt->kern_pairs[index] = ker_value.i32;
				}
				i++;
			}
		}
	}

	return 1;

error:
	return -1;
}

struct fnt
fnt_load(str8 path, struct alloc alloc, struct alloc scratch)
{
	struct fnt res       = {0};
	str8 fnt_ext         = str8_lit(".fnt");
	size widths_size     = FNT_CHAR_MAX;
	size kern_pairs_size = FNT_KERN_PAIRS_MAX;
	res.widths           = arr_new(res.widths, widths_size, alloc);
	res.kern_pairs       = arr_new(res.kern_pairs, kern_pairs_size, alloc);

	if(res.widths == NULL) {
		log_error("fnt", "Failed to alloc widths array ");
		return res;
	}
	if(res.widths == NULL) {
		log_error("fnt", "Failed to alloc kern pairs array ");
		return res;
	}

	arr_header(res.widths)->len     = arr_cap(res.widths);
	arr_header(res.kern_pairs)->len = arr_cap(res.kern_pairs);
	mclr(res.widths, sizeof(*res.widths) * widths_size);
	mclr(res.kern_pairs, sizeof(*res.kern_pairs) * kern_pairs_size);

	assert(str8_ends_with(path, fnt_ext, 0));
	struct sys_full_file_res file_res = sys_load_full_file(path, scratch);
	if(file_res.data == NULL) {
		log_error("fnt", "Failed loading info: %s", path.str);
		return res;
	}
	sys_printf(FILE_AND_LINE);
	char *data          = file_res.data;
	usize size          = file_res.size;
	struct ser_reader r = {
		.data = data,
		.len  = size,
	};

	fnt_read(&r, &res);

	str8 base_name = str8_chop_last_dot(path);
	str8 tex_path  = str8_fmt_push(scratch, "%.*s-table-%d-%d.tex", (i32)base_name.size, base_name.str, res.cell_w, res.cell_h);

	res.t = tex_load(tex_path, alloc);
	if(res.t.px == NULL) {
		log_error("fnt", "Failed loading tex: %s", path.str);
		return res;
	}

	res.grid_w = res.t.w / res.cell_w;
	res.grid_h = res.t.h / res.cell_h;
	return res;
}
