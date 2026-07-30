#pragma once

#include "base/mem.h"
#include "engine/gfx/gfx-defs.h"
#include <jsmn.h>

struct opts_parse_ctx {
	struct alloc alloc;
	str8 json;
	struct sys_opts *data;
	const jsmntok_t *tokens;
	ssize token_count;
};

struct sys_screenshot_opts {
	i32 scale;
	str8 save_path;
	struct gfx_col_pallete colors;
};

struct sys_recording_opts {
	i32 scale;
	str8 save_path;
	i32 seconds_count;
	struct gfx_col_pallete colors;
};

enum sys_video_scaling {
	SYS_VIDEO_SCALING_NONE,

	SYS_VIDEO_SCALING_INTEGER,
	SYS_VIDEO_SCALING_OVERSCALE,
	SYS_VIDEO_SCALING_FIT,

	SYS_VIDEO_SCALING_NUM_COUNT,
};

enum sys_video_filter {
	SYS_VIDEO_FILTER_NONE,

	SYS_VIDEO_FILTER_NEAREST,
	SYS_VIDEO_FILTER_BILINEAR,
	SYS_VIDEO_FILTER_SHARP,

	SYS_VIDEO_FILTER_NUM_COUNT,
};

enum sys_video_display {
	SYS_VIDEO_DISPLAY_NONE,

	SYS_VIDEO_DISPLAY_WINDOWED,
	SYS_VIDEO_DISPLAY_FULLSCREEN,

	SYS_VIDEO_DISPLAY_NUM_COUNT,
};

struct sys_video_opts {
	enum sys_video_scaling scaling;
	enum sys_video_filter filter;
	enum sys_video_display display;
	b32 mouse_capture;
};

struct sys_opts {
	struct sys_video_opts video;
	struct gfx_col_pallete colors;
	struct gfx_col_pallete colors_dbg;
	struct sys_screenshot_opts screentshot;
	struct sys_recording_opts recording;
};

b32 sys_opts_read(struct alloc alloc, jsmn_parser *r, struct sys_opts *data, str8 json, jsmntok_t *tokens, ssize token_count);
struct sys_opts sys_opts_load(struct alloc alloc, struct alloc scratch, str8 org, str8 name);
