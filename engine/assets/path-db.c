#include "path-db.h"

#include "base/arr.h"
#include "base/dbg.h"
#include "base/str.h"

void
path_db_write(struct ser_writer *w, struct path_db db)
{
	dbg_assert(arr_len(db.tags) == arr_len(db.paths));
	ser_write_object(w);

	ser_write_string(w, str8_lit("count"));
	ser_write_i32(w, arr_len(db.paths));

	ser_write_string(w, str8_lit("paths"));
	ser_write_array(w);
	for(ssize i = 0; i < arr_len(db.paths); ++i) {
		ser_write_string(w, db.paths[i]);
	}
	ser_write_end(w);

	ser_write_string(w, str8_lit("tags"));
	ser_write_array(w);
	for(ssize i = 0; i < arr_len(db.tags); ++i) {
		ser_write_u32(w, db.tags[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

struct path_db
path_db_read(struct ser_reader *r, struct alloc alloc)
{
	struct path_db res  = {0};
	struct ser_value db = ser_read(r);
	struct ser_value key, value;

	dbg_assert(db.type == SER_TYPE_OBJECT);

	while(ser_iter_object(r, db, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("count"), 0)) {
			i32 count = ser_get_i32(value);
			res.paths = arr_new(alloc, res.paths, count);
			res.tags  = arr_new(alloc, res.tags, count);
		} else if(str8_match(key.str, str8_lit("paths"), 0)) {
			struct ser_value item;
			while(ser_iter_array(r, value, &item)) {
				dbg_assert(item.type == SER_TYPE_STRING);
				arr_push(res.paths, str8_cpy_push(alloc, item.str));
			}
		} else if(str8_match(key.str, str8_lit("tags"), 0)) {
			struct ser_value item;
			while(ser_iter_array(r, value, &item)) {
				arr_push(res.tags, ser_get_u32(item));
			}
		}
	}

	dbg_assert(arr_len(res.tags) == arr_len(res.paths));
	return res;
}
