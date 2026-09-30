#include "sys/sys-opts.h"

#include "base/arr.h"
#include "base/mathfunc.h"
#include "lib/color.h"
#include "lib/json.h"

#define SYS_RECORDING_SECONDS_DEFAULT 120
#define SYS_RECORDING_SCALE_DEFAULT   1

#define SYS_OPTS_COLOR_PALLETE_KEY       "game-colors"
#define SYS_OPTS_VIDEO_KEY               "video"
#define SYS_OPTS_VIDEO_RESOLUTION_KEY    "resolution"
#define SYS_OPTS_VIDEO_SCALING_KEY       "scaling"
#define SYS_OPTS_VIDEO_FILTER_KEY        "filter"
#define SYS_OPTS_VIDEO_DISPLAY_KEY       "display"
#define SYS_OPTS_VIDEO_MOUSE_CAPTURE_KEY "mouse-capture"

#define SYS_OPTS_RECORDING_KEY               "recording"
#define SYS_OPTS_RECORDING_SECONDS_COUNT_KEY "seconds"
#define SYS_OPTS_RECORDING_SCALE_KEY         "scale"
#define SYS_OPTS_RECORDING_SAVE_PATH_KEY     "save-path"
#define SYS_OPTS_RECORDING_COLOR_PALLETE_KEY "colors"

#define SYS_OPTS_SCREENSHOT_KEY               "screenshot"
#define SYS_OPTS_SCREENSHOT_SAVE_PATH_KEY     "save-path"
#define SYS_OPTS_SCREENSHOT_COLOR_PALLETE_KEY "colors"

#define SYS_OPTS_COLOR_PALLETE_BLACK_KEY "black"
#define SYS_OPTS_COLOR_PALLETE_WHITE_KEY "white"
#define SYS_OPTS_COLOR_PALLETE_CLEAR_KEY "clear"

#define APP_DEFAULT_WHITE 0xA5A5A2FF
#define APP_DEFAULT_BLACK 0x110B0DFF

static const str8 SYS_VIDEO_SCALING_LABELS[SYS_VIDEO_SCALING_NUM_COUNT] = {
	[SYS_VIDEO_SCALING_NONE]      = str8_lit_comp("none"),
	[SYS_VIDEO_SCALING_INTEGER]   = str8_lit_comp("integer"),
	[SYS_VIDEO_SCALING_FIT]       = str8_lit_comp("fit"),
	[SYS_VIDEO_SCALING_OVERSCALE] = str8_lit_comp("overscale"),
};

static const str8 SYS_VIDEO_FILTER_LABELS[SYS_VIDEO_FILTER_NUM_COUNT] = {
	[SYS_VIDEO_FILTER_NONE]     = str8_lit_comp("none"),
	[SYS_VIDEO_FILTER_NEAREST]  = str8_lit_comp("nearest"),
	[SYS_VIDEO_FILTER_BILINEAR] = str8_lit_comp("bilinear"),
	[SYS_VIDEO_FILTER_SHARP]    = str8_lit_comp("sharp"),
};

static const str8 SYS_VIDEO_DISPLAY_LABELS[SYS_VIDEO_DISPLAY_NUM_COUNT] = {
	[SYS_VIDEO_DISPLAY_NONE]       = str8_lit_comp("none"),
	[SYS_VIDEO_DISPLAY_WINDOWED]   = str8_lit_comp("windowed"),
	[SYS_VIDEO_DISPLAY_FULLSCREEN] = str8_lit_comp("fullscreen"),
};

static void sys_opts_cb(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user);

static void sys_opts_parse_color_palette(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user);
static b32 sys_opts_parse_color_palette_key(str8 json, jsmntok_t *key, enum gfx_col *out);

struct sys_opts
sys_opts_load(struct alloc alloc, struct alloc scratch, str8 org, str8 name)
{
	str8 path              = str8_lit("settings.json");
	str8 full_path         = sys_path_to_data_path(scratch, path, org, name);
	str8 default_save_path = sys_path_to_data_path(alloc, str8_lit(""), org, name);
	struct sys_opts res    = {
		.video = {
			.resolution    = {SYS_PD_DISPLAY_W, SYS_PD_DISPLAY_H},
			.scaling       = SYS_VIDEO_SCALING_FIT,
			.filter        = SYS_VIDEO_FILTER_SHARP,
			.display       = SYS_VIDEO_DISPLAY_WINDOWED,
			.mouse_capture = false,
		},

		.colors.colors[GFX_COL_BLACK] = APP_DEFAULT_BLACK,
		.colors.colors[GFX_COL_WHITE] = APP_DEFAULT_WHITE,

		.colors_dbg.colors = {
			0x00000000,
			0xff000080,
			0x00ff0080,
			0x0000ff80,
			0x10c6f880,
			0xaaff7f80,
			0x92ff1880,
			0x55007f80,
			0x8c60a680,
			0xaf1f7d80,
			0x55aaff80,
			0xdb72a080,
			0xbfff1a80,
			0x1e00ff80,
		},

		.recording = {
			.save_path     = default_save_path,
			.scale         = SYS_RECORDING_SCALE_DEFAULT,
			.seconds_count = SYS_RECORDING_SECONDS_DEFAULT,
			.colors.colors = {
				[GFX_COL_BLACK] = 0x000000ff,
				[GFX_COL_WHITE] = 0xffffffff,
			},
		},

		.screentshot = {
			.save_path     = default_save_path,
			.colors.colors = {
				[GFX_COL_BLACK] = 0x000000ff,
				[GFX_COL_WHITE] = 0xffffffff,
			},
		},
	};

	str8 json = {0};
	json_load(full_path, scratch, &json);
	if(json.size == 0) { goto error; }

	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	dbg_check_warn(token_count > 0, "sys-opts", "invalid settings JSON; using defaults");
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_new(scratch, tokens, token_count);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	dbg_check_warn(json_res == token_count, "sys-opts", "invalid settings JSON; using defaults");
	dbg_check_warn(sys_opts_read(alloc, &parser, &res, json, tokens, token_count), "sys-opts", "failed to parse settings");

	str8 recording_colors[GFX_COL_NUM_COUNT] = {
		[GFX_COL_BLACK] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.recording.colors.colors[GFX_COL_BLACK])),
		[GFX_COL_WHITE] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.recording.colors.colors[GFX_COL_WHITE])),
		[GFX_COL_CLEAR] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.recording.colors.colors[GFX_COL_CLEAR])),

	};

	str8 screenshot_colors[GFX_COL_NUM_COUNT] = {
		[GFX_COL_BLACK] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.screentshot.colors.colors[GFX_COL_BLACK])),
		[GFX_COL_WHITE] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.screentshot.colors.colors[GFX_COL_WHITE])),
		[GFX_COL_CLEAR] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.screentshot.colors.colors[GFX_COL_CLEAR])),
	};

	str8 colors[GFX_COL_NUM_COUNT] = {
		[GFX_COL_BLACK] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.colors.colors[GFX_COL_BLACK])),
		[GFX_COL_WHITE] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.colors.colors[GFX_COL_WHITE])),
		[GFX_COL_CLEAR] = color_rgba_to_hex_str(scratch, color_rgba_from_u32(res.colors.colors[GFX_COL_CLEAR])),
	};

	str8 scaling_label = SYS_VIDEO_SCALING_LABELS[res.video.scaling];
	str8 filter_label  = SYS_VIDEO_FILTER_LABELS[res.video.filter];
	str8 display_label = SYS_VIDEO_DISPLAY_LABELS[res.video.display];

	log_info(
		"sys-opts",
		"loaded video: resolution=%dx%d scaling=%.*s filter=%.*s display=%.*s mouse-capture=%s",
		res.video.resolution.x,
		res.video.resolution.y,
		(int)scaling_label.size,
		scaling_label.str,
		(int)filter_label.size,
		filter_label.str,
		(int)display_label.size,
		display_label.str,
		res.video.mouse_capture ? "true" : "false");

	log_info(
		"sys-opts",
		"loaded colors: %.*s,%.*s,%.*s",

		(int)colors[GFX_COL_BLACK].size,
		colors[GFX_COL_BLACK].str,
		(int)colors[GFX_COL_WHITE].size,
		colors[GFX_COL_WHITE].str,
		(int)colors[GFX_COL_CLEAR].size,
		colors[GFX_COL_CLEAR].str);

	log_info(
		"sys-opts",
		"loaded recording:\n"
		" scale: %d\n"
		" seconds: %d\n"
		" colors: %.*s,%.*s,%.*s\n"
		" save path:%.*s",

		(int)res.recording.scale,
		(int)res.recording.seconds_count,

		(int)recording_colors[GFX_COL_BLACK].size,
		recording_colors[GFX_COL_BLACK].str,
		(int)recording_colors[GFX_COL_WHITE].size,
		recording_colors[GFX_COL_WHITE].str,
		(int)recording_colors[GFX_COL_CLEAR].size,
		recording_colors[GFX_COL_CLEAR].str,

		(int)res.recording.save_path.size,
		res.recording.save_path.str);

	log_info(
		"sys-opts",
		"loaded screenshot:\n",
		" colors: %.*s,%.*s,%.*s\n"
		" save path: %.*s",

		(int)screenshot_colors[GFX_COL_BLACK].size,
		screenshot_colors[GFX_COL_BLACK].str,
		(int)screenshot_colors[GFX_COL_WHITE].size,
		screenshot_colors[GFX_COL_WHITE].str,
		(int)screenshot_colors[GFX_COL_CLEAR].size,
		screenshot_colors[GFX_COL_CLEAR].str,
		(int)res.screentshot.save_path.size,
		res.screentshot.save_path.str);

error:;
	return res;
}

static str8
sys_opts_palette_json(struct alloc alloc, const struct gfx_col_palette *palette)
{
	return str8_fmt_push(alloc,
		"{\"" SYS_OPTS_COLOR_PALLETE_BLACK_KEY "\":\"%08x\","
		"\"" SYS_OPTS_COLOR_PALLETE_WHITE_KEY "\":\"%08x\","
		"\"" SYS_OPTS_COLOR_PALLETE_CLEAR_KEY "\":\"%08x\"}",
		palette->colors[GFX_COL_BLACK],
		palette->colors[GFX_COL_WHITE],
		palette->colors[GFX_COL_CLEAR]);
}

b32
sys_opts_write(struct alloc scratch, const struct sys_opts *opts, str8 org, str8 name)
{
	b32 res       = false;
	sys_file file = sys_file_zero();
	str8 path     = sys_path_to_data_path(scratch, str8_lit("settings.json"), org, name);
	str8 path_new = str8_fmt_push(scratch, "%.*s.new", str8_spread(path));

	dbg_check_warn(opts->video.scaling > SYS_VIDEO_SCALING_NONE && opts->video.scaling < SYS_VIDEO_SCALING_NUM_COUNT,
		"sys-opts",
		"invalid scaling mode");

	dbg_check_warn(opts->video.filter > SYS_VIDEO_FILTER_NONE && opts->video.filter < SYS_VIDEO_FILTER_NUM_COUNT,
		"sys-opts",
		"invalid filter mode");

	dbg_check_warn(opts->video.display > SYS_VIDEO_DISPLAY_NONE && opts->video.display < SYS_VIDEO_DISPLAY_NUM_COUNT,
		"sys-opts",
		"invalid display mode");

	str8 recording_path  = json_scape_raw_str8(scratch, opts->recording.save_path);
	str8 screenshot_path = json_scape_raw_str8(scratch, opts->screentshot.save_path);

	dbg_check_warn(recording_path.str && screenshot_path.str, "sys-opts", "cannot encode save paths");

	str8 colors            = sys_opts_palette_json(scratch, &opts->colors);
	str8 recording_colors  = sys_opts_palette_json(scratch, &opts->recording.colors);
	str8 screenshot_colors = sys_opts_palette_json(scratch, &opts->screentshot.colors);

	str8 json = str8_fmt_push(scratch,
		"{\n"
		"  \"" SYS_OPTS_VIDEO_KEY "\": {\n"
		"    \"" SYS_OPTS_VIDEO_RESOLUTION_KEY "\": {\"width\": %d, \"height\": %d},\n"
		"    \"" SYS_OPTS_VIDEO_SCALING_KEY "\": \"%.*s\",\n"
		"    \"" SYS_OPTS_VIDEO_FILTER_KEY "\": \"%.*s\",\n"
		"    \"" SYS_OPTS_VIDEO_DISPLAY_KEY "\": \"%.*s\",\n"
		"    \"" SYS_OPTS_VIDEO_MOUSE_CAPTURE_KEY "\": %s\n"
		"  },\n"
		"  \"" SYS_OPTS_COLOR_PALLETE_KEY "\": %.*s,\n"
		"  \"" SYS_OPTS_RECORDING_KEY "\": {\n"
		"    \"" SYS_OPTS_RECORDING_SCALE_KEY "\": %d,\n"
		"    \"" SYS_OPTS_RECORDING_SECONDS_COUNT_KEY "\": %d,\n"
		"    \"" SYS_OPTS_RECORDING_SAVE_PATH_KEY "\": \"%.*s\",\n"
		"    \"" SYS_OPTS_RECORDING_COLOR_PALLETE_KEY "\": %.*s\n"
		"  },\n"
		"  \"" SYS_OPTS_SCREENSHOT_KEY "\": {\n"
		"    \"" SYS_OPTS_SCREENSHOT_SAVE_PATH_KEY "\": \"%.*s\",\n"
		"    \"" SYS_OPTS_SCREENSHOT_COLOR_PALLETE_KEY "\": %.*s\n"
		"  }\n"
		"}\n",
		opts->video.resolution.x,
		opts->video.resolution.y,
		str8_spread(SYS_VIDEO_SCALING_LABELS[opts->video.scaling]),
		str8_spread(SYS_VIDEO_FILTER_LABELS[opts->video.filter]),
		str8_spread(SYS_VIDEO_DISPLAY_LABELS[opts->video.display]),
		opts->video.mouse_capture ? "true" : "false",
		str8_spread(colors),
		opts->recording.scale,
		opts->recording.seconds_count,
		str8_spread(recording_path),
		str8_spread(recording_colors),
		str8_spread(screenshot_path),
		str8_spread(screenshot_colors));

	dbg_check_warn(json.str && json.size <= U32_MAX, "sys-opts", "failed to serialize, json too big");
	str8 dir = sys_path_to_data_path(scratch, str8_lit(""), org, name);

	dbg_check_warn(dir.size == 0 || sys_make_dir(dir), "sys-opts", "failed to create directory: %s", dir.str);
	file = sys_file_open_w(path_new);

	dbg_check_warn(sys_file_is_valid(file), "sys-opts", "failed to open for writing: %s", path_new.str);
	dbg_check_warn(sys_file_w(file, json.str, (u32)json.size) == (ssize)json.size, "sys-opts", "failed to write: %s", path_new.str);
	dbg_check_warn(sys_file_flush(file), "sys-opts", "failed to flush: %s", path_new.str);

	b32 closed = sys_file_close(file);
	file       = sys_file_zero();
	dbg_check_warn(closed, "sys-opts", "failed to close: %s", path_new.str);

	dbg_check_warn(sys_file_replace(path_new, path), "sys-opts", "failed to replace: %s", path.str);

	log_info("sys-opts", "saved: %s", path.str);
	res = true;

error:;
	if(sys_file_is_valid(file)) sys_file_close(file);
	return res;
}

b32
sys_opts_read(
	struct alloc alloc,
	jsmn_parser *r,
	struct sys_opts *data,
	str8 json,
	jsmntok_t *tokens,
	ssize token_count)
{
	b32 res = false;
	dbg_check_warn(token_count > 0 && tokens[0].type == JSMN_OBJECT, "sys-opts", "invalid settings file");

	struct opts_parse_ctx ctx = {alloc, json, data, tokens, token_count};

	json_obj_foreach(tokens, token_count, 0, sys_opts_cb, &ctx);
	res = true;

error:;
	return res;
}

static void
sys_opts_parse_color_palette(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user)
{
	struct {
		struct opts_parse_ctx *ctx;
		struct gfx_col_palette *pal;
	} *ctx = user;

	str8 json = ctx->ctx->json;

	enum gfx_col col;
	if(sys_opts_parse_color_palette_key(json, key, &col)) {
		str8 str              = json_str8(json, value);
		u32 color             = color_rgba_to_u32(color_rgba_from_hex_str(str));
		ctx->pal->colors[col] = color;
	}
}

static b32
sys_opts_parse_color_palette_key(str8 json, jsmntok_t *key, enum gfx_col *out)
{
	if(json_eq(json, key, str8_lit(SYS_OPTS_COLOR_PALLETE_BLACK_KEY)) == 0) {
		*out = GFX_COL_BLACK;
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_COLOR_PALLETE_WHITE_KEY)) == 0) {
		*out = GFX_COL_WHITE;
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_COLOR_PALLETE_CLEAR_KEY)) == 0) {
		*out = GFX_COL_CLEAR;
	} else {
		return false;
	}
	return true;
}

static void
recording_cb(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user)
{
	struct opts_parse_ctx *ctx = user;
	str8 json                  = ctx->json;
	struct sys_opts *data      = ctx->data;
	struct alloc alloc         = ctx->alloc;

	if(json_eq(json, key, str8_lit(SYS_OPTS_RECORDING_SCALE_KEY)) == 0) {
		data->recording.scale = clamp_i32(json_parse_i32(json, value), 1, 4);
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_RECORDING_SECONDS_COUNT_KEY)) == 0) {
		data->recording.seconds_count = json_parse_i32(json, value);
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_RECORDING_SAVE_PATH_KEY)) == 0) {
		data->recording.save_path = json_str8_cpy_push(json, value, alloc, json_copy_unescape);
	} else if(json_eq(json, key, str8_lit("colors")) == 0) {
		if(value->type == JSMN_OBJECT) {
			struct {
				struct opts_parse_ctx *ctx;
				struct gfx_col_palette *pal;
			} sub = {ctx, &data->recording.colors};

			json_obj_foreach(
				ctx->tokens,
				ctx->token_count,
				value_idx,
				sys_opts_parse_color_palette,
				&sub);
		}
	}
}

static void
screenshot_cb(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user)
{
	struct opts_parse_ctx *ctx = user;
	str8 json                  = ctx->json;
	struct sys_opts *data      = ctx->data;
	struct alloc alloc         = ctx->alloc;

	if(json_eq(json, key, str8_lit(SYS_OPTS_SCREENSHOT_SAVE_PATH_KEY)) == 0) {
		data->screentshot.save_path = json_str8_cpy_push(json, value, alloc, json_copy_unescape);
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_SCREENSHOT_COLOR_PALLETE_KEY)) == 0) {
		if(value->type == JSMN_OBJECT) {
			struct {
				struct opts_parse_ctx *ctx;
				struct gfx_col_palette *pal;
			} sub = {ctx, &data->screentshot.colors};

			json_obj_foreach(
				ctx->tokens,
				ctx->token_count,
				value_idx,
				sys_opts_parse_color_palette,
				&sub);
		}
	}
}

static void
sys_opts_resolution_cb(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user)
{
	struct opts_parse_ctx *ctx = user;
	if(json_eq(ctx->json, key, str8_lit("width")) == 0) {
		ctx->data->video.resolution.x = json_parse_i32(ctx->json, value);
	} else if(json_eq(ctx->json, key, str8_lit("height")) == 0) {
		ctx->data->video.resolution.y = json_parse_i32(ctx->json, value);
	}
	dbg_assert(ctx->data->video.resolution.x > 0 && ctx->data->video.resolution.x <= I16_MAX);
	dbg_assert(ctx->data->video.resolution.y > 0 && ctx->data->video.resolution.y <= I16_MAX);
}

static void
video_cb(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user)
{
	struct opts_parse_ctx *ctx = user;
	str8 json                  = ctx->json;
	struct sys_opts *data      = ctx->data;

	if(json_eq(json, key, str8_lit(SYS_OPTS_VIDEO_RESOLUTION_KEY)) == 0) {
		if(value->type == JSMN_OBJECT) {
			json_obj_foreach(ctx->tokens, ctx->token_count, value_idx, sys_opts_resolution_cb, ctx);
		}
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_VIDEO_SCALING_KEY)) == 0) {
		if(json_eq(json, value, str8_lit("integer")) == 0) {
			data->video.scaling = SYS_VIDEO_SCALING_INTEGER;
		} else if(json_eq(json, value, str8_lit("overscale")) == 0) {
			data->video.scaling = SYS_VIDEO_SCALING_OVERSCALE;
		} else if(json_eq(json, value, str8_lit("fit")) == 0) {
			data->video.scaling = SYS_VIDEO_SCALING_FIT;
		}
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_VIDEO_FILTER_KEY)) == 0) {
		if(json_eq(json, value, str8_lit("nearest")) == 0) {
			data->video.filter = SYS_VIDEO_FILTER_NEAREST;
		} else if(json_eq(json, value, str8_lit("bilinear")) == 0) {
			data->video.filter = SYS_VIDEO_FILTER_BILINEAR;
		} else if(json_eq(json, value, str8_lit("sharp")) == 0) {
			data->video.filter = SYS_VIDEO_FILTER_SHARP;
		}
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_VIDEO_DISPLAY_KEY)) == 0) {
		if(json_eq(json, value, str8_lit("windowed")) == 0) {
			data->video.display = SYS_VIDEO_DISPLAY_WINDOWED;
		} else if(json_eq(json, value, str8_lit("fullscreen")) == 0) {
			data->video.display = SYS_VIDEO_DISPLAY_FULLSCREEN;
		}
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_VIDEO_MOUSE_CAPTURE_KEY)) == 0) {
		data->video.mouse_capture = json_parse_bool32(json, value);
	}
}

static void
sys_opts_cb(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user)
{
	struct opts_parse_ctx *ctx = user;
	struct sys_opts *data      = ctx->data;

	str8 json = ctx->json;

	if(json_eq(json, key, str8_lit(SYS_OPTS_VIDEO_KEY)) == 0) {
		if(value->type == JSMN_OBJECT) {
			json_obj_foreach(
				ctx->tokens,
				ctx->token_count,
				value_idx,
				video_cb,
				user);
		}
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_RECORDING_KEY)) == 0) {
		if(value->type == JSMN_OBJECT) {
			json_obj_foreach(
				ctx->tokens,
				ctx->token_count,
				value_idx,
				recording_cb,
				user);
		}
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_SCREENSHOT_KEY)) == 0) {
		if(value->type == JSMN_OBJECT) {
			json_obj_foreach(
				ctx->tokens,
				ctx->token_count,
				value_idx,
				screenshot_cb,
				user);
		}
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_COLOR_PALLETE_KEY)) == 0) {
		if(value->type == JSMN_OBJECT) {
			struct {
				struct opts_parse_ctx *ctx;
				struct gfx_col_palette *pal;
			} sub = {ctx, &data->colors};

			json_obj_foreach(
				ctx->tokens,
				ctx->token_count,
				value_idx,
				sys_opts_parse_color_palette,
				&sub);
		}
	}
}
