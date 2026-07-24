#include "sys/sys-opts.h"

#include "base/arr.h"
#include "base/mathfunc.h"
#include "lib/color.h"
#include "lib/json.h"

#define SYS_RECORDING_SECONDS_DEFAULT 120
#define SYS_RECORDING_SCALE_DEFAULT   1

#define SYS_OPTS_COLOR_PALLETE_KEY "game-colors"

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
		.colors.colors[GFX_COL_BLACK] = 0x110B0DFF,
		.colors.colors[GFX_COL_WHITE] = 0xA5A5A2FF,

		.colors_dbg.colors[GFX_COL_BLACK] = 0x000000FF,
		.colors_dbg.colors[GFX_COL_WHITE] = 0xFFFFFFFF,

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
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_new(scratch, tokens, token_count);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	dbg_assert(json_res == token_count);
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
		struct gfx_col_pallete *pal;
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
		data->recording.save_path = json_str8_cpy_push(json, value, alloc, 0);
	} else if(json_eq(json, key, str8_lit("colors")) == 0) {
		if(value->type == JSMN_OBJECT) {
			struct {
				struct opts_parse_ctx *ctx;
				struct gfx_col_pallete *pal;
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
		data->screentshot.save_path = json_str8_cpy_push(json, value, alloc, 0);
	} else if(json_eq(json, key, str8_lit(SYS_OPTS_SCREENSHOT_COLOR_PALLETE_KEY)) == 0) {
		if(value->type == JSMN_OBJECT) {
			struct {
				struct opts_parse_ctx *ctx;
				struct gfx_col_pallete *pal;
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
sys_opts_cb(jsmntok_t *key, ssize key_idx, jsmntok_t *value, ssize value_idx, void *user)
{
	struct opts_parse_ctx *ctx = user;
	struct sys_opts *data      = ctx->data;

	str8 json = ctx->json;

	if(json_eq(json, key, str8_lit(SYS_OPTS_RECORDING_KEY)) == 0) {
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
				struct gfx_col_pallete *pal;
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
