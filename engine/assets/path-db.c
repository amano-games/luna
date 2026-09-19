#include "path-db.h"

#include "base/arr.h"
#include "base/dbg.h"
#include "base/str.h"

void
path_db_write(struct ser_writer *w, str8 *paths)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("count"));
	ser_write_i32(w, arr_len(paths));

	ser_write_string(w, str8_lit("paths"));
	ser_write_array(w);
	for(ssize i = 0; i < arr_len(paths); ++i) {
		ser_write_string(w, paths[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

str8 *
path_db_read(struct ser_reader *r, struct alloc alloc)
{
	str8 *res            = NULL;
	struct ser_value db  = ser_read(r);
	struct ser_value key, value;

	dbg_assert(db.type == SER_TYPE_OBJECT);

	while(ser_iter_object(r, db, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("count"), 0)) {
			res = arr_new(alloc, res, ser_get_i32(value));
		} else if(str8_match(key.str, str8_lit("paths"), 0)) {
			struct ser_value item;
			while(ser_iter_array(r, value, &item)) {
				dbg_assert(item.type == SER_TYPE_STRING);
				arr_push(res, str8_cpy_push(alloc, item.str));
			}
		}
	}

	return res;
}
