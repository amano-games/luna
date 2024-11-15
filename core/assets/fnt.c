#include "fnt.h"

#include "arr.h"
#include "serialize/serialize.h"
#include "str.h"
#include "sys-assert.h"

i32
fnt_char_size_x_px(struct fnt fnt, i32 a, i32 b)
{
	i32 x = 0;
	x += fnt.widths[a] ? fnt.widths[a] : fnt.cell_w;
	x += fnt.tracking;
	if(fnt.kern_pairs != NULL && b > 0) {
		u16 kern_i = ((u16)a << 8) | b;
		if(kern_i > 0) {
			x += fnt.kern_pairs[kern_i];
		}
	}

	return x;
}

v2_i32
fnt_size_px(struct fnt fnt, const str8 str)
{
	i32 x = 0;
	for(usize i = 0; i < str.size; i++) {
		i32 ci  = str.str[i];
		i32 cbi = -1;
		if(i < str.size - 1) {
			cbi = str.str[i + 1];
		}
		x += fnt_char_size_x_px(fnt, ci, cbi);
	}
	return (v2_i32){x, fnt.cell_h};
}

v2_i32
fnt_size_px_mono(struct fnt fnt, const str8 str, i32 spacing)
{
	i32 x = 0;
	i32 s = spacing ? spacing : fnt.cell_w;
	for(usize i = 0; i < str.size; i++) {
		x += s;
	}
	return (v2_i32){x, fnt.cell_h};
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
		assert(key.type == SER_TYPE_STRING);
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
				fnt->widths[i] = item_val.u8;
				i++;
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
}
