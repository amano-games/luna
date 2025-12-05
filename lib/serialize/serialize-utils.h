#pragma once

#include "lib/serialize/serialize.h"

void
ser_write_v2(struct ser_writer *w, v2 value)
{
	ser_write_array(w);
	ser_write_f32(w, value.x);
	ser_write_f32(w, value.y);
	ser_write_end(w);
}

v2
ser_read_v2(struct ser_reader *r, struct ser_value arr)
{
	dbg_assert(arr.type == SER_TYPE_ARRAY);
	v2 res                 = {0};
	struct ser_value value = {0};

	ser_iter_array(r, arr, &value);
	res.x = ser_get_f32(value);

	ser_iter_array(r, arr, &value);
	res.y = ser_get_f32(value);

	return res;
}

void
ser_write_v2_i32(struct ser_writer *w, v2_i32 value)
{
	ser_write_array(w);

	ser_write_i32(w, value.x);
	ser_write_i32(w, value.y);

	ser_write_end(w);
}

struct v2_i32
ser_read_v2_i32(struct ser_reader *r, struct ser_value arr)
{
	dbg_assert(arr.type == SER_TYPE_ARRAY);
	struct v2_i32 res      = {0};
	struct ser_value value = {0};

	ser_iter_array(r, arr, &value);
	res.x = ser_get_i32(value);

	ser_iter_array(r, arr, &value);
	res.y = ser_get_i32(value);

	return res;
}
