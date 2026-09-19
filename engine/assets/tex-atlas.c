#include "tex-atlas.h"

#include "base/dbg.h"
#include "base/str.h"

void
atlas_write(struct ser_writer *w, struct tex_atlas atlas)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("cell_width"));
	ser_write_i32(w, atlas.cell_size.x);

	ser_write_string(w, str8_lit("cell_height"));
	ser_write_i32(w, atlas.cell_size.y);

	ser_write_end(w);
}

struct tex_atlas
atlas_read(struct ser_reader *r)
{
	struct tex_atlas res = {0};
	struct ser_value db  = ser_read(r);
	struct ser_value key, value;

	dbg_assert(db.type == SER_TYPE_OBJECT);

	while(ser_iter_object(r, db, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("cell_width"), 0)) {
			res.cell_size.x = ser_get_i32(value);
		} else if(str8_match(key.str, str8_lit("cell_height"), 0)) {
			res.cell_size.y = ser_get_i32(value);
		}
	}

	return res;
}
