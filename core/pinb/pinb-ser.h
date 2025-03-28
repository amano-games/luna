#pragma once

#include "pinb/pinb.h"
#include "serialize/serialize.h"

i32 pinb_write(struct ser_writer *w, struct pinb_table pinb);
i32 pinb_read(struct ser_reader *r, struct ser_value obj, struct pinb_table *table, struct alloc alloc);
