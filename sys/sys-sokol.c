#include "sys-sokol.h"
#include "base/mathfunc.h"
#include "base/marena.h"
#include "sys-debug-draw.h"
#include "base/types.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <tinydir.h>
#if !defined(TARGET_WASM)
#include "whereami.h"
#endif

#include "engine/gfx/gfx.h"
#include "engine/gfx/gfx-defs.h"
#include "base/path.h"
#include "base/str.h"
#include "base/utils.h"
#include "sys-input.h"
#include "sys-io.h"
#include "base/log.h"
#include "sys.h"
#include "base/dbg.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SOKOL_IMPL
#define SOKOL_dbg_assert(c) dbg_assert(c);

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_audio.h"
#include "shaders/sokol_shader.h"

#define SOKOL_TOUCH_INVALID U8_MAX
#define SOKOL_PIXEL_PERFECT

struct touch_point_mouse_emu {
	uintptr_t id;
	sapp_mousebutton btn;
};

static const str8 STEAM_RUNTIME_RELATIVE_PATH = str8_lit_comp("steam-runtime");

struct sokol_state {
	struct marena scratch_marena;
	struct alloc scratch;

	str8 exe_path;
	str8 module_path;
	str8 base_path;

	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;

	f32 mouse_scroll_sensitivity;
	u8 frame_buffer[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];
	u8 debug_buffer[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];

	struct tex debug_t;
	struct gfx_ctx debug_ctx;

	u8 keys[SYS_KEYS_LEN];
	b32 crank_docked;
	f32 crank;
	f32 volume;

	f32 mouse_x;
	f32 mouse_y;
	u32 mouse_btns;
	struct touch_point_mouse_emu touches_mouse[SAPP_MAX_TOUCHPOINTS];
};

static struct sokol_state SOKOL_STATE;
const u32 SOKOL_BW_PAL[2] = {0xA2A5A5, 0x0D0B11};
// const u32 SOKOL_BW_PAL[2]    = {0xFFFFFF, 0x000000};
const u32 SOKOL_DEBUG_PAL[2] = {0xFFFFFF, 0x000000};
const f32 COL_WHITE[3]       = {0.64f, 0.64f, 0.64f};
const f32 COL_BLACK[3]       = {0.05f, 0.04f, 0.06f};
const f32 COL_RED[3]         = {1.0f, 0.0f, 0.0f};
const f32 COL_YELLOW[3]      = {1.0f, 0.784f, 0.2f};
const f32 COL_PURPLE[3]      = {0.424f, 0.0f, 1.0f};

void sokol_init(void);
void sokol_frame(void);
void sokol_event(const sapp_event *ev);
void sokol_stream_cb(f32 *buffer, int num_frames, int num_channels);
void sokol_cleanup(void);

static void sokol_exe_path_set(void);
static void sokol_set_icon(void);
static inline void sokol_tex_to_rgb(const u8 *in, u32 *out, usize size, const u32 *pal);
static inline b32 sokol_touch_add(sapp_touchpoint point, sapp_mousebutton button);
static inline b32 sokol_touch_remove(sapp_touchpoint point);
str8 sokol_path_to_res_path(struct str8 path);
static inline s_buffer_params_t sokol_get_buffer_params(f32 win_w, f32 win_h);

sapp_desc
sokol_main(i32 argc, char **argv)
{
	{
		usize mem_size = MMEGABYTE(1);
		void *mem      = sys_alloc(NULL, mem_size);
		marena_init(&SOKOL_STATE.scratch_marena, mem, mem_size);
		SOKOL_STATE.scratch = marena_allocator(&SOKOL_STATE.scratch_marena);
	}
	sokol_exe_path_set();

	struct str8 exe_path = SOKOL_STATE.exe_path;
	str8 base_name       = str8_chop_last_slash(exe_path);
	log_info("SYS", "dirname:  %.*s", (i32)exe_path.size, exe_path.str);
	log_info("SYS", "basename:  %.*s", (i32)base_name.size, base_name.str);

	{
#if defined TARGET_LINUX
		if(!getenv("STEAM_RUNTIME")) {
			marena_reset(&SOKOL_STATE.scratch_marena);
			struct alloc alloc = SOKOL_STATE.scratch;
			if(exe_path.size != 0) {
				struct str8_list path_list = {0};
				str8_list_push(alloc, &path_list, exe_path);
				str8_list_push(alloc, &path_list, STEAM_RUNTIME_RELATIVE_PATH);
				str8 runtime_path = path_join_by_style(alloc, &path_list, path_style_absolute_unix);
				log_info("SYS", "STEAM_RUNTIME %s", runtime_path.str);
				setenv("STEAM_RUNTIME", (char *)runtime_path.str, 1);
			}
		}
#endif
	}

	return (sapp_desc){
		.width              = SYS_DISPLAY_W * 2,
		.height             = SYS_DISPLAY_H * 2,
		.init_cb            = sokol_init,
		.frame_cb           = sokol_frame,
		.cleanup_cb         = sokol_cleanup,
		.event_cb           = sokol_event,
		.logger.func        = slog_func,
		.icon.sokol_default = true,
		.window_title       = "Devils on the Moon Pinball",
	};
}

void
sokol_init(void)
{
	SOKOL_STATE.crank_docked             = true;
	SOKOL_STATE.mouse_scroll_sensitivity = 0.03f;

	SOKOL_STATE.debug_t       = (struct tex){0};
	SOKOL_STATE.debug_t.fmt   = TEX_FMT_OPAQUE;
	SOKOL_STATE.debug_t.px    = (u32 *)SOKOL_STATE.debug_buffer;
	SOKOL_STATE.debug_t.w     = SYS_DISPLAY_W;
	SOKOL_STATE.debug_t.h     = SYS_DISPLAY_H;
	SOKOL_STATE.debug_t.wword = SYS_DISPLAY_WWORDS;

	SOKOL_STATE.debug_ctx         = (struct gfx_ctx){0};
	SOKOL_STATE.debug_ctx.dst     = SOKOL_STATE.debug_t;
	SOKOL_STATE.debug_ctx.clip_x2 = SOKOL_STATE.debug_t.w - 1;
	SOKOL_STATE.debug_ctx.clip_y2 = SOKOL_STATE.debug_t.h - 1;
	mset(&SOKOL_STATE.debug_ctx.pat, 0xFF, sizeof(struct gfx_pattern));
	for(size i = 0; i < (size)ARRLEN(SOKOL_STATE.touches_mouse); ++i) {
		SOKOL_STATE.touches_mouse[i].id = SOKOL_TOUCH_INVALID;
	}

	stm_setup();
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	saudio_setup(&(saudio_desc){
		.buffer_frames = 256,
		.stream_cb     = sokol_stream_cb,
		.logger.func   = slog_func,
	});

	/* a pass action to framebuffer to black */
	SOKOL_STATE.pass_action = (sg_pass_action){
		.colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = {0.25f, 0.5f, 0.75f, 1.0f}},
	};

	SOKOL_STATE.bind.samplers[0] = sg_make_sampler(&(sg_sampler_desc){
		.label = "sampler",
#if defined(SOKOL_PIXEL_PERFECT)
		.min_filter = SG_FILTER_NEAREST,
		.mag_filter = SG_FILTER_NEAREST,
#else
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
#endif
	});

	sg_image_desc img_desc = {
		.width        = SYS_DISPLAY_W,
		.height       = SYS_DISPLAY_H,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.usage        = {.stream_update = true},
	};

	SOKOL_STATE.bind.images[IMG_tex]       = sg_make_image(&img_desc);
	SOKOL_STATE.bind.images[IMG_tex_debug] = sg_make_image(&img_desc);

	// clang-format off
    const float vertices[] = {
        // pos          // uv
        -1.0f,  1.0f,   0.0, 1.0,
         1.0f,  1.0f,   1.0, 1.0,
         1.0f, -1.0f,   1.0, 0.0,
        -1.0f, -1.0f,   0.0, 0.0,
    };
	// We need 2 triangles for a square, this makes 6 indexes.
	const u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };
	// clang-format on

	SOKOL_STATE.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.data  = SG_RANGE(vertices),
		.label = "quad-vertices",
	});

	SOKOL_STATE.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
		.usage = {.index_buffer = true},
		.data  = SG_RANGE(indices),
		.label = "quad-indices",
	});

	sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

	SOKOL_STATE.pip = sg_make_pipeline(&(sg_pipeline_desc){
		.label  = "pipeline",
		.shader = shd,
		// If the vertex layout doesn't have gaps, there is no need to provide strides and offsets.
		.layout = {
			.attrs = {
				[ATTR_simple_pos].format       = SG_VERTEXFORMAT_FLOAT2,
				[ATTR_simple_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
			}},
		.index_type = SG_INDEXTYPE_UINT16,
		.cull_mode  = SG_CULLMODE_NONE,
	});

	sapp_show_mouse(true);
	sokol_set_icon();
	sys_internal_init();
}

void
sokol_event(const sapp_event *ev)
{
	switch(ev->type) {
	case SAPP_EVENTTYPE_KEY_DOWN: {
		SOKOL_STATE.keys[ev->key_code] = 1;
		switch(ev->key_code) {
		case SAPP_KEYCODE_ESCAPE: {
		} break;
		case SAPP_KEYCODE_R: {
#if defined(TARGET_MACOS)
			if(ev->modifiers & SAPP_MODIFIER_SUPER) {
#else
			if(ev->modifiers & SAPP_MODIFIER_CTRL) {
#endif
				sys_internal_init();
			}
		} break;
		case SAPP_KEYCODE_Q: {
#if defined(TARGET_MACOS)
			if(ev->modifiers & SAPP_MODIFIER_SUPER) {
				sapp_request_quit();
			}
#endif
		} break;
		default: {
		} break;
		}
	} break;
	case SAPP_EVENTTYPE_KEY_UP: {
		SOKOL_STATE.keys[ev->key_code] = 0;
	} break;
	case SAPP_EVENTTYPE_MOUSE_SCROLL: {
		SOKOL_STATE.crank_docked = false;
		SOKOL_STATE.crank += ev->scroll_y * -SOKOL_STATE.mouse_scroll_sensitivity;
		SOKOL_STATE.crank = fmodf(SOKOL_STATE.crank, 1.0f);
	} break;
	case SAPP_EVENTTYPE_MOUSE_MOVE: {
		f32 mx                          = ev->mouse_x;
		f32 my                          = ev->mouse_y;
		f32 win_w                       = ev->window_width;
		f32 win_h                       = ev->window_height;
		struct s_buffer_params_t params = sokol_get_buffer_params(win_w, win_h);
		f32 rel_x                       = clamp_f32((mx - params.offset.x) / params.scale.x, 0, SYS_DISPLAY_W);
		f32 rel_y                       = clamp_f32((my - params.offset.y) / params.scale.y, 0, SYS_DISPLAY_H);

		SOKOL_STATE.mouse_x = rel_x;
		SOKOL_STATE.mouse_y = rel_y;
	} break;
	case SAPP_EVENTTYPE_MOUSE_DOWN: {
		SOKOL_STATE.mouse_btns |= 1 << ev->mouse_button;
	} break;
	case SAPP_EVENTTYPE_MOUSE_UP: {
		SOKOL_STATE.mouse_btns &= ~(1 << ev->mouse_button);
	} break;
	case SAPP_EVENTTYPE_TOUCHES_BEGAN: {
		for(size i = 0; i < ev->num_touches; ++i) {
			struct sapp_touchpoint touch = ev->touches[i];
			if(touch.pos_y > ev->window_height * 0.8f) {
				sokol_touch_add(touch, SAPP_MOUSEBUTTON_MIDDLE);
			} else {
				if(touch.pos_x < ev->window_width * 0.5f) {
					sokol_touch_add(touch, SAPP_MOUSEBUTTON_LEFT);
				} else {
					sokol_touch_add(touch, SAPP_MOUSEBUTTON_RIGHT);
				}
			}
		}
	} break;
	case SAPP_EVENTTYPE_TOUCHES_ENDED: {
		for(size i = 0; i < ev->num_touches; ++i) {
			struct sapp_touchpoint touch = ev->touches[i];
			sokol_touch_remove(touch);
		}
	} break;
	default: {
	} break;
	}
}

#define F32_SCALE (1.0f / I16_MAX)
void
sokol_stream_cb(f32 *buffer, int num_frames, int num_channels)
{
	dbg_assert(1 == num_channels);
	b32 is_mono = (num_channels == 1);

	static i16 lbuf[0x1000];
	static i16 rbuf[0x1000];
	mclr_array(lbuf);
	mclr_array(rbuf);

	sys_internal_audio(lbuf, rbuf, num_frames);

	f32 *s     = buffer;
	i16 *l     = lbuf;
	i16 *r     = rbuf;
	f32 volume = 0.1f;

	for(i32 n = 0; n < num_frames; n++) {
		f32 vl = (*l++ * F32_SCALE) * volume;                // Convert and apply volume for left channel
		f32 vr = is_mono ? vl : (*r++ * F32_SCALE) * volume; // Convert and apply volume for right channel (or mono)

		// Store the f32 values in the output stream buffer
		*s++ = vl; // Left channel
		if(num_channels == 2) {
			*s++ = vr; // Right channel, only if stereo
		}
	}
}

void
sokol_frame(void)
{
	f32 win_w                                            = sapp_widthf();
	f32 win_h                                            = sapp_heightf();
	s_params_t params                                    = {.time = sys_seconds()};
	s_buffer_params_t buffer_params                      = sokol_get_buffer_params(win_w, win_h);
	s_colors_t colors                                    = {0};
	u32 *pixels[SYS_DISPLAY_W * SYS_DISPLAY_H * 4]       = {0};
	u32 *pixels_debug[SYS_DISPLAY_W * SYS_DISPLAY_H * 4] = {0};
	usize size                                           = ARRLEN(pixels);

	mcpy_array(colors.color_black, COL_BLACK);
	mcpy_array(colors.color_white, COL_WHITE);
	mcpy_array(colors.color_debug, COL_RED);

#if defined SOKOL_PIXEL_PERFECT
	params.pixel_perfect = true;
#else
	params.pixel_perfect = false;
#endif

	// mcpy_array(colors.color_black, COL_PURPLE);
	// mcpy_array(colors.color_white, COL_PURPLE);
	// mcpy_array(colors.color_debug, COL_RED);

	sokol_tex_to_rgb(SOKOL_STATE.frame_buffer, (u32 *)pixels, size, SOKOL_BW_PAL);
	sokol_tex_to_rgb(SOKOL_STATE.debug_buffer, (u32 *)pixels_debug, size, SOKOL_DEBUG_PAL);

	sg_update_image(
		SOKOL_STATE.bind.images[IMG_tex],
		&(sg_image_data){
			.subimage[0][0] = {
				.ptr  = pixels,
				.size = size,
			},
		});

	sg_update_image(
		SOKOL_STATE.bind.images[IMG_tex_debug],
		&(sg_image_data){
			.subimage[0][0] = {
				.ptr  = pixels_debug,
				.size = size,
			},
		});

	sg_begin_pass(&(sg_pass){
		.action    = SOKOL_STATE.pass_action,
		.swapchain = sglue_swapchain(),
	});
	sg_apply_pipeline(SOKOL_STATE.pip);
	sg_apply_bindings(&SOKOL_STATE.bind);
	sg_apply_uniforms(UB_s_params, &SG_RANGE(params));
	sg_apply_uniforms(UB_s_colors, &SG_RANGE(colors));
	sg_apply_uniforms(UB_s_buffer_params, &SG_RANGE(buffer_params));
	sg_draw(0, 6, 1);
	sg_end_pass();
	sg_commit();
	sys_internal_update();
}

void
sokol_cleanup(void)
{
	if(SOKOL_STATE.scratch_marena.buf_og != NULL) { sys_free(SOKOL_STATE.scratch_marena.buf_og); }
	if(SOKOL_STATE.exe_path.size > 0) { sys_free(SOKOL_STATE.exe_path.str); }
	if(SOKOL_STATE.module_path.size > 0) { sys_free(SOKOL_STATE.module_path.str); }
	if(SOKOL_STATE.base_path.size > 0) { sys_free(SOKOL_STATE.base_path.str); }
	sg_shutdown();
	saudio_shutdown();
	sys_internal_close();
}

struct str8
sys_base_path(void)
{
	return SOKOL_STATE.base_path;
}

str8
sys_exe_path(void)
{
	return SOKOL_STATE.exe_path;
}

// https://wiki.libsdl.org/SDL3/SDL_GetPrefPath
str8
sys_pref_path(void)
{
	return str8_lit("");
}

i32
sys_inp(void)
{
	i32 b    = 0;
	u8 *keys = SOKOL_STATE.keys;

	if(keys[SAPP_KEYCODE_W]) b |= SYS_INP_DPAD_U;
	if(keys[SAPP_KEYCODE_S]) b |= SYS_INP_DPAD_D;
	if(keys[SAPP_KEYCODE_A]) b |= SYS_INP_DPAD_L;
	if(keys[SAPP_KEYCODE_D]) b |= SYS_INP_DPAD_R;
	if(keys[SAPP_KEYCODE_PERIOD]) b |= SYS_INP_A;
	if(keys[SAPP_KEYCODE_COMMA]) b |= SYS_INP_B;

	if(keys[SAPP_KEYCODE_UP]) b |= SYS_INP_DPAD_U;
	if(keys[SAPP_KEYCODE_DOWN]) b |= SYS_INP_DPAD_D;
	if(keys[SAPP_KEYCODE_LEFT]) b |= SYS_INP_DPAD_L;
	if(keys[SAPP_KEYCODE_RIGHT]) b |= SYS_INP_DPAD_R;
	if(keys[SAPP_KEYCODE_X]) b |= SYS_INP_A;
	if(keys[SAPP_KEYCODE_Z]) b |= SYS_INP_B;

	if(keys[SAPP_KEYCODE_SPACE]) b |= SYS_INP_A;

	if(keys[SAPP_KEYCODE_Q]) b |= SYS_INP_A;
	if(keys[SAPP_KEYCODE_E]) b |= SYS_INP_B;

	u32 mouse_btns = SOKOL_STATE.mouse_btns;
	if((SOKOL_STATE.mouse_btns & (1 << SAPP_MOUSEBUTTON_LEFT)) == (1 << SAPP_MOUSEBUTTON_LEFT)) {
		b |= SYS_INP_MOUSE_LEFT;
	}
	if((SOKOL_STATE.mouse_btns & (1 << SAPP_MOUSEBUTTON_RIGHT)) == (1 << SAPP_MOUSEBUTTON_RIGHT)) {
		b |= SYS_INP_MOUSE_RIGHT;
	}
	if((SOKOL_STATE.mouse_btns & (1 << SAPP_MOUSEBUTTON_MIDDLE)) == (1 << SAPP_MOUSEBUTTON_MIDDLE)) {
		b |= SYS_INP_MOUSE_MIDDLE;
	}

	return b;
}

int
sys_key(i32 key)
{
	return SOKOL_STATE.keys[key];
}

void
sys_keys(u8 *dest, usize size)
{
	mcpy(dest, SOKOL_STATE.keys, sizeof(SOKOL_STATE.keys));
}

f32
sys_crank(void)
{
	return SOKOL_STATE.crank;
}

i32
sys_crank_docked(void)
{
	return SOKOL_STATE.crank_docked;
}

f32
sys_mouse_x(void)
{
	return SOKOL_STATE.mouse_x;
}

f32
sys_mouse_y(void)
{
	return SOKOL_STATE.mouse_y;
}

f32
sys_seconds(void)
{
	return stm_sec(stm_since(0));
}

#define SECONDS_BETWEEN_1970_AND_2000 946684800LL

// TODO: Win32 support
// Returns seconds since 2000-01-01 UTC.
// If milliseconds != NULL, stores the 0–999 ms remainder.
u32
sys_epoch_2000(u32 *milliseconds)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	u64 unix_seconds = (u64)ts.tv_sec;
	u64 seconds      = unix_seconds - SECONDS_BETWEEN_1970_AND_2000;

	if(milliseconds) {
		*milliseconds = (u32)(ts.tv_nsec / 1000000ULL); // 0–999 ms
	}

	return seconds;
}

void
sys_1bit_invert(b32 i)
{
	dbg_not_implemeneted("sokol");

error:
	return;
}

void *
sys_1bit_buffer(void)
{
	return (u32 *)SOKOL_STATE.frame_buffer;
}

struct alloc
sys_allocator(void)
{
	struct alloc alloc = {
		.allocf = sys_alloc,
		.ctx    = NULL,
	};
	return alloc;
}

void *
sys_alloc(void *ptr, usize size)
{
	void *res = malloc(size);
	dbg_check(res, "sys-sokol", "Alloc failed to get %" PRIu32 ", %$$u", size, (uint)size);

error:
	return res;
}

void
sys_free(void *ptr)
{
	free(ptr);
}

void
sys_log(
	const char *tag,
	enum sys_log_level log_level,
	u32 log_item,
	const char *msg,
	u32 line_nr,
	const char *filename)
{
	if(log_level <= SYS_LOG_LEVEL) {
		slog_func(tag, log_level, log_item, msg, line_nr, filename, NULL);
	}
}

long
sys_sokol_file_size_get(const str8 path)
{
	FILE *fp = sys_file_open_r(path);

	if(fp == NULL)
		return -1;

	if(fseek(fp, 0, SEEK_END) < 0) {
		fclose(fp);
		return -1;
	}

	long size = sys_file_tell(fp);
	// release the resources when not required
	fclose(fp);
	return size;
}

struct sys_file_stats
sys_file_stats(str8 path)
{
	// TODO: Fill the other stats
	struct sys_file_stats res = {0};
	int size                  = sys_sokol_file_size_get(path);
	if(size < 0) {
		log_error("IO", "failed to get file stats %s", path.str);
	}
	res.size = size;
	return res;
}

void *
sys_file_open_r(const str8 path)
{
	void *res = (void *)fopen((char *)path.str, "rb");
	return res;
}

void *
sys_file_open_w(const str8 path)
{
	void *res = (void *)fopen((char *)path.str, "w");
	return res;
}

void *
sys_file_open_a(const str8 path)
{
	void *res = (void *)fopen((char *)path.str, "ab");
	return res;
}

i32
sys_file_close(void *f)
{
	return fclose((FILE *)f);
}

i32
sys_file_flush(void *f)
{
	return fflush((FILE *)f);
}

i32
sys_file_r(void *f, void *buf, u32 buf_size)
{
	i32 count = 1;
	usize s   = fread(buf, buf_size, count, (FILE *)f);
	if(s == 0) {
		log_error("IO", "Error reading from file: %d", (int)s);
	}

	return (i32)s;
}

i32
sys_file_w(void *f, const void *buf, u32 buf_size)
{
	int count = 1;
	usize s   = fwrite(buf, buf_size, count, (FILE *)f);
	return (i32)s;
}

i32
sys_file_tell(void *f)
{
	usize t = ftell((FILE *)f);
	return (i32)t;
}

i32
sys_file_seek_set(void *f, i32 pos)
{
	return (i32)fseek((FILE *)f, pos, SEEK_SET);
}

i32
sys_file_seek_cur(void *f, i32 pos)
{
	return (i32)fseek((FILE *)f, pos, SEEK_CUR);
}

i32
sys_file_seek_end(void *f, i32 pos)
{
	return (i32)fseek((FILE *)f, pos, SEEK_END);
}

b32
sys_file_del(str8 path)
{
	return 0;
}

b32
sys_file_rename(str8 from, str8 to)
{
	return (rename((char *)from.str, (char *)to.str) == 0);
}

void
sys_set_auto_lock_disabled(int disable)
{
}

void
sys_menu_item_add(int id, const char *title, void (*callback)(void *arg), void *arg)
{
}

void
sys_menu_checkmark_add(int id, const char *title, int val, void (*callback)(void *arg), void *arg)
{
}

void
sys_menu_options_add(int id, const char *title, const char **options, int count, void (*callback)(void *arg), void *arg)
{
}

int
sys_menu_value(int id)
{
	return 0;
}

void
sys_menu_clr(void)
{
}

void
sys_draw_debug_clear(void)
{
}

void
sys_debug_draw(struct debug_shape *shapes, int count)
{
#if DEBUG
	struct gfx_ctx ctx = SOKOL_STATE.debug_ctx;
	tex_clr(ctx.dst, GFX_COL_BLACK);

	for(int i = 0; i < count; ++i) {
		struct debug_shape *shape = &shapes[i];
		switch(shape->type) {
		case DEBUG_CIR: {
			struct debug_shape_cir cir = shape->cir;
			if(cir.filled) {
				gfx_cir_fill(ctx, cir.p.x, cir.p.y, cir.d, 1);
			} else {
				gfx_cir(ctx, cir.p.x, cir.p.y, cir.d, 1);
			}
		} break;
		case DEBUG_REC: {
			struct debug_shape_rec rec = shape->rec;
			if(rec.filled) {
				gfx_rec_fill(ctx, rec.x, rec.y, rec.w, rec.h, 1);
			} else {
				gfx_rec(ctx, rec.x, rec.y, rec.w, rec.h, 1);
			}
		} break;
		case DEBUG_POLY: {
			dbg_sentinel("sokol");
		} break;
		case DEBUG_LIN: {
			struct debug_shape_lin lin = shape->lin;
			gfx_lin(ctx, lin.a.x, lin.a.y, lin.b.x, lin.b.y, 1);
		} break;
		case DEBUG_ELLIPSIS: {
			struct debug_shape_ellipsis ellipsis = shape->ellipsis;
			gfx_ellipsis(ctx, ellipsis.x, ellipsis.y, ellipsis.rx, ellipsis.ry, 1);
		} break;
		default: {
		} break;
		}
	}

error:
	return;

#endif
}

void
sys_audio_set_volume(f32 vol)
{
	sys_audio_lock();
	sys_audio_unlock();
}

f32
sys_audio_get_volume(void)
{
	return SOKOL_STATE.volume;
}

void
sys_audio_lock(void)
{
	// SDL_LockAudioDevice(g_SDL.audiodevID);
	return;
}

void
sys_audio_unlock(void)
{
	// SDL_UnlockAudioDevice(g_SDL.audiodevID);
	return;
}

usize
sys_file_modified(str8 path)
{
	dbg_not_implemeneted("sokol");

error:
	return 0;
}

static inline void
sokol_tex_to_rgb(const u8 *in, u32 *out, usize size, const u32 *pal)
{
	u32 *pixels = out;
	for(i32 y = 0; y < SYS_DISPLAY_H; y++) {
		for(i32 x = 0; x < SYS_DISPLAY_W; x++) {
			i32 i     = (x >> 3) + y * SYS_DISPLAY_WBYTES;
			i32 k     = x + y * SYS_DISPLAY_W;
			i32 byt   = in[i];
			i32 bit   = !!(byt & 0x80 >> (x & 7));
			pixels[k] = pal[!bit];
		}
	}
}

void
sys_set_menu_image(void *px, int h, int wbyte, i32 x_offset)
{
	dbg_not_implemeneted("sokol");

error:
	return;
}

int
sys_score_add(str8 board_id, u32 value)
{
	dbg_not_implemeneted("sokol");

error:
	return 0;
}

int
sys_scores_get(str8 board_id)
{
	dbg_not_implemeneted("sokol");

error:
	return 0;
}

static void
sokol_exe_path_set(void)
{
	{
		str8 res        = {0};
		i32 dirname_len = 0;

#if !defined(TARGET_WASM)
		res.size = wai_getExecutablePath(NULL, 0, &dirname_len);
#endif

		if(res.size > 0) {
			res.str = (u8 *)sys_alloc(NULL, res.size + 1);
#if !defined(TARGET_WASM)
			wai_getExecutablePath((char *)res.str, res.size, &dirname_len);
#endif

			res.str[res.size] = '\0';
		}
		SOKOL_STATE.exe_path = res;
	}
	{
		str8 res        = {0};
		i32 dirname_len = 0;

#if !defined(TARGET_WASM)
		res.size = wai_getModulePath(NULL, 0, &dirname_len);
#endif

		if(res.size > 0) {
			res.str = (u8 *)sys_alloc(NULL, res.size + 1);
#if !defined(TARGET_WASM)
			wai_getModulePath((char *)res.str, res.size, &dirname_len);
#endif

			res.str[res.size] = '\0';
		}
		SOKOL_STATE.module_path = res;
	}

	{
#if defined(TARGET_MACOS)
		marena_reset(&SOKOL_STATE.scratch_marena);
		struct alloc scratch = SOKOL_STATE.scratch;
		struct alloc alloc   = sys_allocator();
		str8 exe_path        = SOKOL_STATE.exe_path;
		if(exe_path.size > 0) {
			// /Users/mariocarballozama/projects/games/devils-on-the-moon-pinball/build/macos/devils-on-the-moon-pinball.app/Contents/MacOS/devils-on-the-moon-pinball
			// /Users/mariocarballozama/projects/games/devils-on-the-moon-pinball/build/macos/devils-on-the-moon-pinball.app/Contents/MacOS
			// /Users/mariocarballozama/projects/games/devils-on-the-moon-pinball/build/macos/devils-on-the-moon-pinball.app/Contents
			str8 macos                 = str8_chop_last_slash(exe_path);
			str8 contents              = str8_chop_last_slash(macos);
			str8 resources_rel         = str8_lit("Resources");
			enum path_style path_style = path_style_from_str8(resources_rel);
			struct str8_list path_list = {0};
			str8_list_push(scratch, &path_list, contents);
			str8_list_push(scratch, &path_list, resources_rel);
			str8 resources_path   = path_join_by_style(alloc, &path_list, path_style);
			SOKOL_STATE.base_path = resources_path;
		}
#else
		SOKOL_STATE.base_path = (str8){0};
#endif
	}
}

static inline b32
sokol_touch_add(sapp_touchpoint point, sapp_mousebutton button)
{
	b32 res = false;

	// Make sure the point doesn't already exist
	for(size i = 0; i < (size)ARRLEN(SOKOL_STATE.touches_mouse); ++i) {
		struct touch_point_mouse_emu emu = SOKOL_STATE.touches_mouse[i];
		dbg_check(emu.id != point.identifier, "sokol", "Touch point already exists %" PRIxPTR "", emu.id);
	}

	for(size i = 0; i < (size)ARRLEN(SOKOL_STATE.touches_mouse); ++i) {
		struct touch_point_mouse_emu emu = SOKOL_STATE.touches_mouse[i];
		if(emu.id == SOKOL_TOUCH_INVALID) {
			SOKOL_STATE.touches_mouse[i].id  = point.identifier;
			SOKOL_STATE.touches_mouse[i].btn = button;
			SOKOL_STATE.mouse_btns |= 1 << button;
			res = true;
			break;
		}
	}

error:;
	return res;
}

static inline b32
sokol_touch_remove(sapp_touchpoint point)
{
	b32 res = false;
	for(size i = 0; i < (size)ARRLEN(SOKOL_STATE.touches_mouse); ++i) {
		struct touch_point_mouse_emu emu = SOKOL_STATE.touches_mouse[i];
		sapp_mousebutton btn             = SOKOL_STATE.touches_mouse[i].btn;
		if(emu.id == point.identifier) {
			SOKOL_STATE.mouse_btns &= ~(1 << btn);
			SOKOL_STATE.touches_mouse[i].id = SOKOL_TOUCH_INVALID;
			SOKOL_STATE.touches_mouse[i]
				.btn = 0;
			res      = true;
			break;
		}
	}

	return res;
}

static void
sokol_set_icon(void)
{
	marena_reset(&SOKOL_STATE.scratch_marena);
	str8 icons_dir           = sokol_path_to_res_path(str8_lit("icons"));
	tinydir_dir dir          = {0};
	struct alloc scratch     = SOKOL_STATE.scratch;
	str8 png                 = str8_lit(".png");
	sapp_icon_desc icon_desc = {.sokol_default = true};

	tinydir_open(&dir, (char *)icons_dir.str);
	log_info("sokol", "loading icons from: %s", icons_dir.str);

	i32 icon_count = 0;
	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		tinydir_next(&dir);

		if(file.is_dir) { continue; }

		str8 file_name = str8_cstr(file.name);

		if(!str8_ends_with(file_name, png, 0)) { continue; }

		i32 underscore_p = str8_find_needle(file_name, 0, str8_lit("_"), 0);
		if(underscore_p == 0) { continue; }

		str8 size_str = str8_chop_last_dot(str8_skip(file_name, underscore_p + 1));
		i32 icon_size = str8_to_i32(size_str);

		struct str8_list path_list = {0};
		enum path_style path_style = path_style_from_str8(icons_dir);
		str8_list_push(scratch, &path_list, icons_dir);
		str8_list_push(scratch, &path_list, file_name);
		str8 full_path = path_join_by_style(scratch, &path_list, path_style);

		log_info("sokol", "Found icon of size %d: %.*s", icon_size, (i32)full_path.size, full_path.str);

		i32 w, h, n;
		uint32_t *data = (uint32_t *)stbi_load((char *)full_path.str, &w, &h, &n, 4);
		if(data == NULL) { continue; }
		dbg_assert(w == icon_size);
		dbg_assert(h == icon_size);

		sapp_image_desc img            = {.height = h, .width = w, .pixels = {.size = w * h * n, .ptr = data}};
		icon_desc.images[icon_count++] = img;
		log_info("sokol", "Loaded icon of size %d loaded: %.*s", icon_size, (i32)full_path.size, full_path.str);
	}

	if(icon_count > 0) {
		icon_desc.sokol_default = false;
		sapp_set_icon(&icon_desc);
	}

	for(size i = 0; i < icon_count; ++i) {
		stbi_image_free((char *)icon_desc.images[i].pixels.ptr);
	}
}

str8
sokol_path_to_res_path(struct str8 path)
{
	str8 res       = path;
	str8 base_path = sys_base_path();
	if(base_path.size == 0) { return res; }

	marena_reset(&SOKOL_STATE.scratch_marena);
	enum path_style path_style = path_style_from_str8(path);
	struct alloc scratch       = SOKOL_STATE.scratch;
	struct str8_list path_list = {0};
	str8_list_push(scratch, &path_list, base_path);
	str8_list_push(scratch, &path_list, path);
	res = path_join_by_style(scratch, &path_list, path_style);

	return res;
}

static inline s_buffer_params_t
sokol_get_buffer_params(f32 win_w, f32 win_h)
{
	s_buffer_params_t res = {0};
	res.win_size.x        = win_w;
	res.win_size.y        = win_h;
	res.app_size.x        = SYS_DISPLAY_W;
	res.app_size.y        = SYS_DISPLAY_H;
	f32 win_aspect        = res.win_size.x / res.win_size.y;
	f32 app_aspect        = res.app_size.x / res.app_size.y;
	f32 scale             = 1.0f;
	if(win_aspect > app_aspect) {
		// window is wider -> fit height
		scale = res.win_size.y / res.app_size.y;
	} else {
		// window is taller/narrower -> fit width
		scale = res.win_size.x / res.app_size.x;
	}
#if defined(SOKOL_PIXEL_PERFECT)
	scale = floor_f32(scale);
#endif
	scale        = max_f32(scale, 1.0f);
	res.scale.x  = scale;
	res.scale.y  = scale;
	res.size.x   = res.app_size.x * res.scale.x;
	res.size.y   = res.app_size.y * res.scale.y;
	res.offset.x = (res.win_size.x - res.size.x) * 0.5f;
	res.offset.y = (res.win_size.y - res.size.y) * 0.5f;
#if defined(SOKOL_PIXEL_PERFECT)
	res.offset.x = floor_f32(res.offset.x);
	res.offset.y = floor_f32(res.offset.y);
#endif

	dbg_assert(res.scale.x != 0.0f);
	dbg_assert(res.scale.y != 0.0f);
	return res;
}
