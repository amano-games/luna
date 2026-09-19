#pragma once

#include "base/mem.h"
#include "base/str.h"
#include "lib/serialize/serialize.h"

#define ADB_EXT "adb"

void path_db_write(struct ser_writer *w, str8 *paths);
str8 *path_db_read(struct ser_reader *r, struct alloc alloc);
