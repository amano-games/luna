#pragma once

#include "base/date-time.h"
#include "base/dbg.h"
#include "base/mem.h"
#include "base/types.h"
#include "sys/sys.h"

struct recording_1b {
	ssize idx;
	ssize len;
	ssize cap;
	struct tex *frames;
};

struct recording_aud {
	ssize idx;
	ssize len;
	ssize cap;
	f32 *frames;
};

struct sys_recording {
	struct recording_1b gfx;
	struct recording_aud aud;
};

b32
recording_1b_ini(
	struct alloc alloc,
	struct recording_1b *gfx,
	ssize count)
{
	b32 res     = false;
	gfx->cap    = count;
	gfx->len    = 0;
	gfx->idx    = 0;
	gfx->frames = alloc_arr(alloc, gfx->frames, gfx->cap);
	dbg_check_mem(gfx->frames, "sys-recording");
	for(ssize i = 0; i < gfx->cap; ++i) {
		gfx->frames[i] = tex_create_opaque(alloc, SYS_DISPLAY_W, SYS_DISPLAY_H);
	}
	res = true;
error:;
	return res;
}

b32
recording_aud_ini(
	struct alloc alloc,
	struct recording_aud *gfx,
	ssize count)
{
	b32 res     = false;
	gfx->cap    = count;
	gfx->len    = 0;
	gfx->idx    = 0;
	gfx->frames = alloc_arr_clr(alloc, gfx->frames, gfx->cap);
	dbg_check_mem(gfx->frames, "sys-recording");
	res = true;
error:;
	return res;
}

static inline void
recording_1b_record(struct recording_1b *rec, struct tex *src)
{
	struct tex *dst = rec->frames + rec->idx;
	tex_cpy(dst, src);
	rec->idx = (rec->idx + 1) % rec->cap;
	rec->len = MIN(rec->len + 1, rec->cap);
}

// https://github.com/tsoding/rendering-video-in-c-with-ffmpeg/blob/master/ffmpeg_linux.c
void
sys_recording_write(
	struct alloc scratch,
	struct recording_1b *recording,
	i32 scale,
	struct gfx_col_pallete colors,
	str8 path)
{
	if(!recording || recording->len == 0) return;

	int w = recording->frames[0].w;
	int h = recording->frames[0].h;

	FILE *pipe = NULL;

	// Construct ffmpeg command
	i32 fps                   = sys_ups_target_get();
	struct str8_list cmd_list = {0};
	str8_list_pushf(scratch, &cmd_list, "ffmpeg");
#if BUILD_DEBUG
	str8_list_pushf(scratch, &cmd_list, "-loglevel verbose");
	// str8_list_pushf(scratch, &cmd_list, "-report");
#endif
	str8_list_pushf(scratch, &cmd_list, "-y");

	str8_list_pushf(scratch, &cmd_list, "-f rawvideo");
	str8_list_pushf(scratch, &cmd_list, "-pix_fmt rgba");
	str8_list_pushf(scratch, &cmd_list, "-s %dx%d", w, h);
	str8_list_pushf(scratch, &cmd_list, "-r %d", fps);
	str8_list_pushf(scratch, &cmd_list, "-i -");

	str8_list_pushf(scratch, &cmd_list, "-s %dx%d", w * scale, h * scale);
	str8_list_pushf(scratch, &cmd_list, "-sws_flags neighbor");
	str8_list_pushf(scratch, &cmd_list, "-c:v libx264");
	str8_list_pushf(scratch, &cmd_list, "-pix_fmt yuv420p");
	str8_list_pushf(scratch, &cmd_list, "-vb 2500k");

	str8_list_pushf(scratch, &cmd_list, "\"%s\"", path.str);

	struct str_join params = {.sep = str8_lit(" ")};
	str8 cmd               = str8_list_join(scratch, &cmd_list, &params);
	ssize dst_size         = w * h * sizeof(u32);
	u32 *dst               = alloc_arr(scratch, dst, w * h);
	log_info("sokol-sys", "ffmpeg command: %s\n", cmd.str);

	pipe = popen((char *)cmd.str, "w");
	dbg_check_warn(pipe, "sokol", "Failed to open pipe to ffmpeg cmd: %s", cmd.str);

	// Write frames in chronological order (handles circular buffer)
	ssize oldest = (recording->idx + recording->cap - (recording->len - 1)) % recording->cap;
	for(ssize i = 0; i < (ssize)recording->len; i++) {
		ssize f        = (oldest + i) % recording->cap;
		struct tex src = recording->frames[f];
		tex_opaque_to_rgba(src, dst, dst_size, colors);
		fwrite(dst, sizeof(u32), w * h, pipe);
	}

error:;
	if(pipe) {
		pclose(pipe);
	}
}
