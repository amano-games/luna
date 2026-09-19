#pragma once

#include "engine/animation/animation.h"
#include "base/mem.h"
#include "lib/serialize/serialize.h"

#define ANI_EXT "ani"

void ani_clips_write(struct ser_writer *w, struct animation_clip *clips);
struct animation_clip *ani_clips_read(struct ser_reader *r, struct alloc alloc);
