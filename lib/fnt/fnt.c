#include "fnt.h"

#include "base/arr.h"
#include "base/mathfunc.h"
#include "lib/serialize/serialize.h"
#include "base/str.h"
#include "sys/sys-log.h"
#include "base/dbg.h"
#include "sys/sys-io.h"

i32
fnt_char_size_x_px(struct fnt fnt, i32 a, i32 b, i32 tracking)
{
	if(a < 32) return 0;

	i32 is_last_char = (b == '\n') || (b == -1);
	i32 x            = fnt_glyph(fnt, a).w;

	if(b > 0) { x += fnt_kern(fnt, a, b); }

	x += (1 - is_last_char) * (tracking + fnt.tracking);

	return x;
}

i32
fnt_mono_size_x_px(struct fnt fnt, str8 str, i32 tracking)
{
	if(str.size == 0) return 0;

	return str.size * fnt.cell_w + (str.size - 1) * tracking;
}

v2_i32
fnt_size_px(struct fnt fnt, const str8 str, i32 tracking, i32 leading)
{
	v2_i32 res = {0};
	i32 x      = 0;
	i32 maxx   = 0;
	i32 y      = fnt.cell_h + leading;
	for(usize i = 0; i < str.size; i++) {
		i32 ci         = str.str[i];
		i32 cbi        = (i < str.size - 1) ? str.str[i + 1] : -1;
		i32 is_newline = (ci == '\n');
		i32 move_x     = fnt_char_size_x_px(fnt, ci, cbi, tracking);
		x              = (1 - is_newline) * (x + move_x);
		maxx           = max_i32(maxx, x);
		y += is_newline * (fnt.cell_h + leading);
	}
	res = (v2_i32){maxx, y};
	return res;
}

struct fnt_ascii *
fnt_ascii_ini(struct alloc alloc, struct fnt *fnt)
{
	if(fnt->ascii) { return fnt->ascii; }

	fnt->ascii = alloc_struct_clr(alloc, fnt->ascii);
	if(fnt->ascii == NULL) { return NULL; }

	for(i32 c = FNT_ASCII_KERN_MIN; c < FNT_ASCII_MAX; ++c) {
		fnt->ascii->cells[c] = c - FNT_ASCII_KERN_MIN;
	}
	return fnt->ascii;
}

b32
fnt_ascii_has_glyphs(struct fnt fnt)
{
	if(fnt.ascii == NULL) { return false; }
	for(usize i = 0; i < ARRLEN(fnt.ascii->mask); ++i) {
		if(fnt.ascii->mask[i]) { return true; }
	}
	return false;
}

static void
fnt_write_triple(struct ser_writer *w, i32 a, i32 b, i32 c)
{
	ser_write_array(w);
	ser_write_i32(w, a);
	ser_write_i32(w, b);
	ser_write_i32(w, c);
	ser_write_end(w);
}

static void
fnt_read_triple(struct ser_reader *r, struct ser_value arr, i32 *out)
{
	struct ser_value item;
	for(i32 i = 0; i < 3 && ser_iter_array(r, arr, &item); ++i) {
		out[i] = ser_get_i32(item);
	}
}

struct fnt_utf8 *
fnt_utf8_ini(struct alloc alloc, struct fnt *fnt)
{
	if(fnt->utf8 == NULL) { fnt->utf8 = alloc_struct_clr(alloc, fnt->utf8); }
	return fnt->utf8;
}

void
fnt_write(struct fnt fnt, struct ser_writer *w)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("version"));
	ser_write_i32(w, FNT_FILE_VERSION);
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

	if(fnt_ascii_has_glyphs(fnt)) {
		// [code point, cell, width]
		ser_write_string(w, str8_lit("ascii_glyphs"));
		ser_write_array(w);
		for(u32 c = 0; c < FNT_ASCII_MAX; ++c) {
			if(fnt_ascii_has(fnt, c)) { fnt_write_triple(w, c, fnt.ascii->cells[c], fnt.ascii->widths[c]); }
		}
		ser_write_end(w);

		// [a, b, kerning]
		ser_write_string(w, str8_lit("ascii_kern"));
		ser_write_array(w);
		for(u32 a = FNT_ASCII_KERN_MIN; fnt.ascii->kern && a < FNT_ASCII_MAX; ++a) {
			for(u32 b = FNT_ASCII_KERN_MIN; b < FNT_ASCII_MAX; ++b) {
				i8 kern = fnt.ascii->kern[fnt_ascii_kern_idx(a, b)];
				if(kern != 0) { fnt_write_triple(w, a, b, kern); }
			}
		}
		ser_write_end(w);
	}

	if(fnt.utf8 && fnt.utf8->count > 0) {
		ser_write_string(w, str8_lit("utf8_glyph_count"));
		ser_write_i32(w, fnt.utf8->count);
		// [code point, cell, width], sorted by code point
		ser_write_string(w, str8_lit("utf8_glyphs"));
		ser_write_array(w);
		for(u32 i = 0; i < fnt.utf8->count; ++i) {
			fnt_write_triple(w, fnt.utf8->code_points[i], fnt.utf8->cells[i], fnt.utf8->widths[i]);
		}
		ser_write_end(w);
	}

	if(fnt.utf8 && fnt.utf8->kern_count > 0) {
		ser_write_string(w, str8_lit("utf8_kern_count"));
		ser_write_i32(w, fnt.utf8->kern_count);
		// [a, b, kerning], sorted by a then b
		ser_write_string(w, str8_lit("utf8_kern"));
		ser_write_array(w);
		for(u32 i = 0; i < fnt.utf8->kern_count; ++i) {
			u64 key = fnt.utf8->kern_keys[i];
			fnt_write_triple(w, (u32)(key >> 32), (u32)key, fnt.utf8->kern_vals[i]);
		}
		ser_write_end(w);
	}

	ser_write_end(w);
}

i32
fnt_read(struct alloc alloc, struct ser_reader *r, struct fnt *fnt)
{
	struct ser_value obj = ser_read(r);
	struct ser_value key, value;
	i32 version = 0;

	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_check(key.type == SER_TYPE_STRING, "fnt", "Corrupt fnt data");
		if(str8_match(key.str, str8_lit("version"), 0)) {
			version = ser_get_i32(value);
			dbg_check(version == FNT_FILE_VERSION, "fnt", "Unsupported fnt version %d, expected %d", (int)version, FNT_FILE_VERSION);
		} else if(str8_match(key.str, str8_lit("tracking"), 0)) {
			fnt->tracking = ser_get_i32(value);
		} else if(str8_match(key.str, str8_lit("grid_w"), 0)) {
			fnt->grid_w = ser_get_i32(value);
		} else if(str8_match(key.str, str8_lit("grid_h"), 0)) {
			fnt->grid_h = ser_get_i32(value);
		} else if(str8_match(key.str, str8_lit("cell_w"), 0)) {
			fnt->cell_w = ser_get_i32(value);
		} else if(str8_match(key.str, str8_lit("cell_h"), 0)) {
			fnt->cell_h = ser_get_i32(value);
		} else if(str8_match(key.str, str8_lit("metrics"), 0)) {
			dbg_assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				dbg_assert(item_key.type == SER_TYPE_STRING);
				if(str8_match(item_key.str, str8_lit("baseline"), 0)) {
					fnt->metrics.baseline = ser_get_i32(item_value);
				} else if(str8_match(item_key.str, str8_lit("x_height"), 0)) {
					fnt->metrics.x_height = ser_get_i32(item_value);
				} else if(str8_match(item_key.str, str8_lit("cap_height"), 0)) {
					fnt->metrics.cap_height = ser_get_i32(item_value);
				} else if(str8_match(item_key.str, str8_lit("descent"), 0)) {
					fnt->metrics.descent = ser_get_i32(item_value);
				}
			}
		} else if(str8_match(key.str, str8_lit("ascii_glyphs"), 0)) {
			dbg_check(fnt_ascii_ini(alloc, fnt), "fnt", "Failed to alloc ascii glyphs");
			struct ser_value item;
			while(ser_iter_array(r, value, &item)) {
				i32 t[3] = {0};
				fnt_read_triple(r, item, t);
				dbg_assert(t[0] >= 0 && t[0] < FNT_ASCII_MAX);
				fnt->ascii->cells[t[0]]  = t[1];
				fnt->ascii->widths[t[0]] = t[2];
				fnt->ascii->mask[t[0] / 64] |= 1ull << (t[0] % 64);
			}
		} else if(str8_match(key.str, str8_lit("ascii_kern"), 0)) {
			dbg_check(fnt_ascii_ini(alloc, fnt), "fnt", "Failed to alloc ascii glyphs");
			struct ser_value item;
			while(ser_iter_array(r, value, &item)) {
				if(fnt->ascii->kern == NULL) {
					fnt->ascii->kern = alloc_arr_clr(alloc, fnt->ascii->kern, FNT_ASCII_KERN_MAX * FNT_ASCII_KERN_MAX);
					dbg_check(fnt->ascii->kern, "fnt", "Failed to alloc ascii kern");
				}
				i32 t[3] = {0};
				fnt_read_triple(r, item, t);
				dbg_assert(t[0] >= FNT_ASCII_KERN_MIN && t[0] < FNT_ASCII_MAX && t[1] >= FNT_ASCII_KERN_MIN && t[1] < FNT_ASCII_MAX);
				fnt->ascii->kern[fnt_ascii_kern_idx(t[0], t[1])] = t[2];
			}
		} else if(str8_match(key.str, str8_lit("utf8_glyph_count"), 0)) {
			i32 count = ser_get_i32(value);
			dbg_check(fnt_utf8_ini(alloc, fnt), "fnt", "Failed to alloc utf8 glyphs");
			fnt->utf8->code_points = alloc_arr(alloc, fnt->utf8->code_points, count);
			fnt->utf8->cells       = alloc_arr(alloc, fnt->utf8->cells, count);
			fnt->utf8->widths      = alloc_arr(alloc, fnt->utf8->widths, count);
			dbg_check(fnt->utf8->code_points && fnt->utf8->cells && fnt->utf8->widths, "fnt", "Failed to alloc %d utf8 glyphs", (int)count);
			fnt->utf8->count = count;
		} else if(str8_match(key.str, str8_lit("utf8_glyphs"), 0)) {
			dbg_check(fnt->utf8, "fnt", "utf8_glyphs before utf8_glyph_count");
			struct ser_value item;
			for(u32 i = 0; i < fnt->utf8->count && ser_iter_array(r, value, &item); ++i) {
				i32 t[3] = {0};
				fnt_read_triple(r, item, t);
				fnt->utf8->code_points[i] = t[0];
				fnt->utf8->cells[i]       = t[1];
				fnt->utf8->widths[i]      = t[2];
			}
		} else if(str8_match(key.str, str8_lit("utf8_kern_count"), 0)) {
			i32 count = ser_get_i32(value);
			dbg_check(fnt_utf8_ini(alloc, fnt), "fnt", "Failed to alloc utf8 glyphs");
			fnt->utf8->kern_keys = alloc_arr(alloc, fnt->utf8->kern_keys, count);
			fnt->utf8->kern_vals = alloc_arr(alloc, fnt->utf8->kern_vals, count);
			dbg_check(fnt->utf8->kern_keys && fnt->utf8->kern_vals, "fnt", "Failed to alloc %d utf8 kern pairs", (int)count);
			fnt->utf8->kern_count = count;
		} else if(str8_match(key.str, str8_lit("utf8_kern"), 0)) {
			dbg_check(fnt->utf8, "fnt", "utf8_kern before utf8_kern_count");
			struct ser_value item;
			for(u32 i = 0; i < fnt->utf8->kern_count && ser_iter_array(r, value, &item); ++i) {
				i32 t[3] = {0};
				fnt_read_triple(r, item, t);
				fnt->utf8->kern_keys[i] = ((u64)(u32)t[0] << 32) | (u32)t[1];
				fnt->utf8->kern_vals[i] = t[2];
			}
		}
	}

	dbg_check(version == FNT_FILE_VERSION, "fnt", "Missing fnt version");

	return 1;

error:
	return -1;
}

struct fnt
fnt_load_from_mem(struct alloc alloc, void *data, usize size)
{
	struct fnt res = {0};

	if(data == NULL || size == 0) {
		log_error("fnt", "Failed loading info from mem");
		return res;
	}

	struct ser_reader r = {
		.data = data,
		.len  = size,
	};
	if(fnt_read(alloc, &r, &res) < 0) {
		log_error("fnt", "Failed reading fnt data");
		return (struct fnt){0};
	}

	return res;
}

struct fnt
fnt_load(str8 path, struct alloc alloc, struct alloc scratch)
{
	struct fnt res = {0};
	str8 fnt_ext   = str8_lit(".fnt");

	dbg_assert(str8_ends_with(path, fnt_ext, 0));
	struct sys_full_file_res file_res = sys_load_full_file(scratch, path);
	if(file_res.data == NULL) {
		log_error("fnt", "Failed loading info: %.*s", str8_spread(path));
		return res;
	}

	res = fnt_load_from_mem(alloc, file_res.data, file_res.size);
	if(res.cell_h == 0) {
		return res;
	}

	str8 base_name = str8_chop_last_dot(path);
	str8 tex_path  = str8_fmt_push(scratch, "%.*s-table-%d-%d.tex", str8_spread(base_name), res.cell_w, res.cell_h);

	res.t = tex_load(alloc, scratch, tex_path);
	if(res.t.px1b == NULL) {
		log_error("fnt", "Failed loading tex: %.*s", str8_spread(path));
		return res;
	}

	res.grid_w = res.t.w / res.cell_w;
	res.grid_h = res.t.h / res.cell_h;
	return res;
}

struct str8_list
fnt_wrap_lines_from_str(
	struct alloc alloc,
	struct fnt fnt,
	str8 str,
	i32 max_width,
	i32 tracking,
	i32 leading)
{
	struct str8_list list = {0};
	if(str.str == NULL || str.size == 0) {
		return list;
	}

	union rng_u64 line_rng = rng_u64(0, 0);
	i32 line_w             = 0;
	i32 last_break         = -1;
	i32 last_break_w       = 0;

	for(usize i = 0; i <= str.size; i++) {
		u8 c           = (i < str.size) ? str.str[i] : 0;
		i32 next       = (i + 1 < str.size) ? str.str[i + 1] : -1;
		i32 is_newline = (c == '\n');
		i32 is_end     = (i == str.size);

		if(is_newline || is_end) {
			// push current line (excluding newline)
			union rng_u64 rng = rng_u64(line_rng.min, i);
			str8 line         = str8_substr(str, rng);
			str8_list_push(alloc, &list, line);
			line_rng     = rng_u64(i + 1, i + 1);
			line_w       = 0;
			last_break   = -1;
			last_break_w = 0;
			continue;
		}

		i32 cw = fnt_char_size_x_px(fnt, c, next, tracking);
		line_w += cw;

		if(char_is_space(c)) {
			last_break   = (i32)i;
			last_break_w = line_w;
		}

		if(line_w > max_width && last_break >= 0) {
			// wrap at last break
			union rng_u64 rng = rng_u64(line_rng.min, (u64)last_break);
			str8 line         = str8_substr(str, rng);
			str8_list_push(alloc, &list, line);

			// reset to after last break
			line_rng     = rng_u64((u64)last_break + 1, (u64)last_break + 1);
			i            = (usize)last_break;
			line_w       = line_w - last_break_w;
			last_break   = -1;
			last_break_w = 0;
		}
	}

	// Handle trailing text
	if(line_rng.min < str.size) {
		str8 line = str8_substr(str, line_rng);
		str8_list_push(alloc, &list, line);
	}

	return list;
}
