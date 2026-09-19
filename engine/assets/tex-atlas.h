#pragma once

#include "engine/assets/asset-db.h"
#include "lib/serialize/serialize.h"

#define ATLAS_EXT "atlas"

void atlas_write(struct ser_writer *w, struct tex_atlas atlas);
struct tex_atlas atlas_read(struct ser_reader *r);
