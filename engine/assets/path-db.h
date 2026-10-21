#pragma once

#include "base/mem.h"
#include "base/str.h"
#include "lib/serialize/serialize.h"

#define ADB_EXT "adb"

struct path_db {
	str8 *paths;
	u32 *tags;
};

void path_db_write(struct ser_writer *w, struct path_db db);
struct path_db path_db_read(struct ser_reader *r, struct alloc alloc);
