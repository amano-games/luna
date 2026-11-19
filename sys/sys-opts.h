#pragma once

#include "base/mem.h"
#include "base/str.h"
#include "engine/gfx/gfx-defs.h"

struct sys_screenshot_opts {
	i32 scale;
	str8 save_path;
	struct gfx_col_palette colors;
};

enum sys_recording_format {
	SYS_RECORDING_FORMAT_NONE,

	SYS_RECORDING_FORMAT_MP4,
	SYS_RECORDING_FORMAT_GIF,

	SYS_RECORDING_FORMAT_NUM_COUNT,
};

enum sys_recording_compression {
	SYS_RECORDING_COMPRESSION_NONE,

	SYS_RECORDING_COMPRESSION_LOSSY,
	SYS_RECORDING_COMPRESSION_LOSSLESS,

	SYS_RECORDING_COMPRESSION_NUM_COUNT,
};

struct sys_recording_opts {
	enum sys_recording_format format;
	enum sys_recording_compression compression;
	i32 scale;
	str8 save_path;
	i32 seconds_count;
	struct gfx_col_palette colors;
};

enum sys_video_scaling {
	SYS_VIDEO_SCALING_NONE,

	SYS_VIDEO_SCALING_INTEGER,
	SYS_VIDEO_SCALING_FIT,
	SYS_VIDEO_SCALING_OVERSCALE,

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
	v2_i32 resolution;
	enum sys_video_scaling scaling;
	enum sys_video_filter filter;
	enum sys_video_display display;
	b32 mouse_capture;
};

static const str8 SYS_RECORDING_FORMAT_LABELS[SYS_RECORDING_FORMAT_NUM_COUNT] = {
	[SYS_RECORDING_FORMAT_NONE] = str8_lit_comp("none"),
	[SYS_RECORDING_FORMAT_MP4]  = str8_lit_comp("mp4"),
	[SYS_RECORDING_FORMAT_GIF]  = str8_lit_comp("gif"),
};

static const str8 SYS_RECORDING_COMPRESSION_LABELS[SYS_RECORDING_COMPRESSION_NUM_COUNT] = {
	[SYS_RECORDING_COMPRESSION_NONE]     = str8_lit_comp("none"),
	[SYS_RECORDING_COMPRESSION_LOSSY]    = str8_lit_comp("lossy"),
	[SYS_RECORDING_COMPRESSION_LOSSLESS] = str8_lit_comp("lossless"),
};

#define SYS_OPTS_DEFAULT_FILENAME "settings-luna.json"
#define SYS_AUDIO_VOLUME_DEFAULT  0.31622777f

struct sys_audio_opts {
	f32 volume;
};

struct sys_opts {
	struct sys_audio_opts audio;
	struct sys_video_opts video;
	struct gfx_col_palette colors;
	struct gfx_col_palette colors_dbg;
	struct sys_screenshot_opts screentshot;
	struct sys_recording_opts recording;
};

b32 sys_opts_write(struct alloc scratch, const struct sys_opts *opts, str8 path, str8 org, str8 name);
struct sys_recording_opts sys_recording_opts_default(void);
struct sys_screenshot_opts sys_screenshot_opts_default(void);
struct sys_opts sys_opts_load(struct alloc alloc, struct alloc scratch, str8 path, str8 org, str8 name);
