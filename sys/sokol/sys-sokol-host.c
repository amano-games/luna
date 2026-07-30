#include "sys/sokol/sys-sokol.h"
#include "sys/sys-os.h"
#include "base/mathfunc.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/rec.h"
#include "engine/gfx/gfx-spr.h"
#include "engine/gfx/gfx-txt.h"
#include "lib/color.h"
#include "lib/fnt/fnt.h"
#include "lib/rndm.h"
#include "lib/tex/tex.h"
#include "sys/sys-debug-draw.h"
#include "base/types.h"
#include "sys/sys-font-mono.h"
#include "sys/sys-opts.h"
#include "sys/sys-scoreboards.h"

#include <jsmn.h>
#include <stdio.h>
#include <tinydir.h>

#include "engine/gfx/gfx.h"
#include "engine/gfx/gfx-defs.h"
#include "base/path.h"
#include "base/str.h"
#include "base/utils.h"
#include "sys/sys-input.h"
#include "sys/sys-io.h"
#include "base/log.h"
#include "sys/sys.h"
#include "base/dbg.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SOKOL_IMPL
#define SOKOL_dbg_assert(c) dbg_assert(c);

#if OS_MACOS || OS_WASM
#define MINI_GAMEPAD_ENABLE 0
#else
#define MINI_GAMEPAD_ENABLE 1
#endif

#if MINI_GAMEPAD_ENABLE
#define MG_IMPLEMENTATION
#include "minigamepad.h"
#endif

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_audio.h"
#include "shaders/sokol_shader.h"

#define SOKOL_TOUCH_INVALID U8_MAX
#if OS_WINDOWS
// #define SOKOL_DISABLE_AUDIO
#endif

// #define SOKOL_DBG_AUDIO
// #define SOKOL_AUDIO_FRAMES        256
#define SOKOL_AUDIO_CHANNEL_COUNT 1
#define SOKOL_AUDIO_VOLUME        0.1f
#define SOKOL_AUDIO_BUFFER_CAP    0x1000

#define SOKOL_RECORDING_ENABLED
#define SOKOL_MOCK_PLAYER_NAME "afk"

#if OS_WINDOWS
#undef SOKOL_RECORDING_ENABLED
#endif

struct touch_point_mouse_emu {
	uintptr_t id;
	sapp_mousebutton btn;
};

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

enum sokol_status {
	SOKOL_STATUS_NONE,

	SOKOL_STATUS_INI,
	SOKOL_STATUS_PAUSED,
	SOKOL_STATUS_RELOAD,

	SOKOL_STATUS_NUM_COUNT,
};

struct sokol_paused_state {
	f32 timestamp;
	i32 x_offset;
	struct tex frame_tex;
	struct tex menu_tex;
	struct gfx_ctx ctx;
};

enum sokol_menu_item_type {
	SOKOL_MENU_ITEM_TYPE_NONE,

	SOKOL_MENU_ITEM_TYPE_ACTION,
	SOKOL_MENU_ITEM_TYPE_BOOL,

	SOKOL_MENU_ITEM_TYPE_NUM_COUNT,
};

struct sokol_menu_item {
	i32 id;
	enum sokol_menu_item_type type;
	str8 title;
	i32 value;
	void (*callback)(void *arg);
	void *arg;
};

struct sokol_menu {
	i32 next_id;
	i32 idx;
	i32 len;
	struct sokol_menu_item items[3];
};

struct sokol_state {
	enum sokol_status status;

	struct marena scratch_marena;
	struct alloc scratch;

	struct marena marena;
	struct alloc alloc;

	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
	sg_sampler smp_nearest;
	sg_sampler smp_linear;

	f32 mouse_scroll_sensitivity;

	struct sokol_paused_state paused_state;

	struct gfx_ctx frame_ctx;
	struct gfx_ctx debug_ctx;

	struct sokol_menu menu;

	u8 keys[SYS_KEYS_LEN];
	b32 crank_docked;
	f32 crank;
	f32 volume;

	f32 mouse_x;
	f32 mouse_y;
	u32 mouse_btns;

#if MINI_GAMEPAD_ENABLE
	mg_gamepads gamepads;
#endif

	struct fnt fnt;
	struct sys_opts opts;

	struct recording_1b recording;
	struct recording_aud recording_aud;
	struct touch_point_mouse_emu touches_mouse[SAPP_MAX_TOUCHPOINTS];
};

static struct sokol_state SOKOL_STATE;
static u32 *SOKOL_PIXELS[SYS_DISPLAY_W * SYS_DISPLAY_H * 4]       = {0};
static u32 *SOKOL_PIXELS_DEBUG[SYS_DISPLAY_W * SYS_DISPLAY_H * 4] = {0};

#define SOKOL_ORG       "amano"
#define SOKOL_NAME      "luna"
#define SOKOL_WHITE     "#A2A5A5"
#define SOKOL_BLACK     "#0D0B11"
#define SOKOL_DBG_WHITE "FFFFFF00"
#define SOKOL_DBG_BLACK "00000000"

const f32 COL_WHITE[3]  = {0.64f, 0.64f, 0.64f};
const f32 COL_BLACK[3]  = {0.05f, 0.04f, 0.06f};
const f32 COL_RED[3]    = {1.0f, 0.0f, 0.0f};
const f32 COL_YELLOW[3] = {1.0f, 0.784f, 0.2f};
const f32 COL_PURPLE[3] = {0.424f, 0.0f, 1.0f};

void sokol_init(void);
void sokol_frame(void);
void sokol_event(const sapp_event *ev);
void sokol_stream_cb(f32 *buffer, int num_frames, int num_channels);
void sokol_cleanup(void);

void sokol_pause_handle_sokol_event(const sapp_event *ev);
#if MINI_GAMEPAD_ENABLE
void sokol_pause_handle_gamepad_event(const mg_event *ev);
#endif
void sokol_pause_handle_buttons(i32 buttons);

void sokol_pause(void);
void sokol_resume(void);

static void sokol_set_icon(void);
static inline b32 sokol_touch_add(sapp_touchpoint point, sapp_mousebutton button);
static inline b32 sokol_touch_remove(sapp_touchpoint point);
static void sokol_screenshot_save(struct tex tex);
static void sokol_recording_write(struct recording_1b *recording);
str8 sokol_path_to_res_path(struct str8 path);
static inline s_buffer_params_t sokol_get_buffer_params(f32 win_w, f32 win_h);

#if MINI_GAMEPAD_ENABLE
static inline i32 sokol_gamepads_upd(void);
static inline void sokol_gamepads_ev(void);
#endif

sapp_desc
sokol_main(i32 argc, char **argv)
{
	sys_os_init();
	SOKOL_STATE.menu.next_id = 1;
	{
		usize mem_size = MMEGABYTE(1);
		void *mem      = sys_alloc(NULL, mem_size, 4);
		marena_init(&SOKOL_STATE.scratch_marena, mem, mem_size);
		SOKOL_STATE.scratch = marena_allocator(&SOKOL_STATE.scratch_marena);
	}
	{
		usize mem_size = MMEGABYTE(200);
		void *mem      = sys_alloc(NULL, mem_size, 4);
		marena_init(&SOKOL_STATE.marena, mem, mem_size);
		SOKOL_STATE.alloc = marena_allocator(&SOKOL_STATE.marena);
	}

	struct str8 exe_path = sys_exe_path();
	str8 base_name       = str8_chop_last_slash(exe_path);
	log_info("SYS", "dirname:  %.*s", (i32)exe_path.size, exe_path.str);
	log_info("SYS", "basename:  %.*s", (i32)base_name.size, base_name.str);

	{
		struct sys_opts *opts = &SOKOL_STATE.opts;
		*opts                 = sys_opts_load(SOKOL_STATE.alloc, SOKOL_STATE.scratch, str8_lit(SOKOL_ORG), str8_lit(SOKOL_NAME));
	}

	{
		struct tex tex        = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, SOKOL_STATE.alloc);
		SOKOL_STATE.frame_ctx = gfx_ctx_default(tex);
		dbg_check(tex.px, "sokol", "Failed to create frame buffer");
	}

	{
		struct tex tex        = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, SOKOL_STATE.alloc);
		SOKOL_STATE.debug_ctx = gfx_ctx_default(tex);
		dbg_check(tex.px, "sokol", "Failed to create debug buffer");
	}

	{
		struct tex tex               = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, SOKOL_STATE.alloc);
		SOKOL_STATE.paused_state.ctx = gfx_ctx_default(tex);
		dbg_check(tex.px, "sokol", "Failed to create paused gfx ctx");
	}
	{
		struct tex tex                     = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, SOKOL_STATE.alloc);
		SOKOL_STATE.paused_state.frame_tex = tex;
		dbg_check(tex.px, "sokol", "Failed to create paused frame tex");
	}
	{
		struct tex tex                    = tex_create(SYS_DISPLAY_W, SYS_DISPLAY_H, SOKOL_STATE.alloc);
		SOKOL_STATE.paused_state.menu_tex = tex;
		dbg_check(tex.px, "sokol", "Failed to create menu tex");
	}

#if defined(SOKOL_RECORDING_ENABLED)
	// TODO: use sys_ups_target_get and when fps is changed change recording
	u32 ups = SYS_DEFAULT_UPS;
	{
		struct recording_1b *rec = &SOKOL_STATE.recording;
		struct alloc alloc       = SOKOL_STATE.alloc;
		rec->cap                 = ups * SOKOL_STATE.opts.recording.seconds_count;
		rec->len                 = 0;
		rec->idx                 = 0;
		rec->frames              = alloc_arr(alloc, rec->frames, rec->cap);
		for(ssize i = 0; i < rec->cap; ++i) {
			rec->frames[i] = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, alloc);
		}
		dbg_check_warn(rec->frames != NULL, "sokol", "Failed to reserve recording video memory");
	}
	{
		struct recording_aud *rec = &SOKOL_STATE.recording_aud;
		struct alloc alloc        = SOKOL_STATE.alloc;
		rec->cap                  = ups * SOKOL_STATE.opts.recording.seconds_count;
		rec->len                  = 0;
		rec->idx                  = 0;
		rec->frames               = alloc_arr(alloc, rec->frames, rec->cap);
		for(ssize i = 0; i < rec->cap; ++i) {
			rec->frames[i] = 0;
		}
		dbg_check_warn(rec->frames != NULL, "sokol", "Failed to reserve recording audio memory");
	}
#endif

	{
		str8 dir_path = sys_path_to_data_path(
			SOKOL_STATE.scratch,
			str8_lit(""),
			str8_lit(SOKOL_ORG),
			str8_lit(SOKOL_NAME));
		sys_make_dir(dir_path);
	}

	SOKOL_STATE.fnt = (struct fnt){
		.cell_h             = 9,
		.cell_w             = 6,
		.grid_h             = 12,
		.grid_w             = 10,
		.metrics.baseline   = 8,
		.metrics.x_height   = -1,
		.metrics.cap_height = -1,
		.metrics.descent    = -1,
		.t.wword            = 4,
		.t.fmt              = 1,
		.t.w                = 60,
		.t.h                = 108,
		.t.px               = (u32 *)SYS_MONO_FONT,
	};
	SOKOL_STATE.status = SOKOL_STATUS_INI;

error:;
	sapp_desc res = {
		.width              = SYS_DISPLAY_W * 2,
		.height             = SYS_DISPLAY_H * 2,
		.init_cb            = sokol_init,
		.frame_cb           = sokol_frame,
		.cleanup_cb         = sokol_cleanup,
		.event_cb           = sokol_event,
		.logger.func        = slog_func,
		.icon.sokol_default = true,
		.window_title       = SOKOL_NAME,
	};
	log_info("SYS", "init");
	return res;
}

void
sokol_init(void)
{
	SOKOL_STATE.crank_docked             = true;
	SOKOL_STATE.mouse_scroll_sensitivity = 0.03f;

	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

#if !defined(SOKOL_DISABLE_AUDIO)
	saudio_setup(&(saudio_desc){
		// .buffer_frames = SOKOL_AUDIO_FRAMES,
		.logger.func = slog_func,
		.stream_cb   = sokol_stream_cb,
	});
#endif

	/* a pass action to framebuffer to black */
	SOKOL_STATE.pass_action = (sg_pass_action){
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {
				0.25f,
				0.5f,
				0.75f,
				1.0f,
			},
		},
	};

	SOKOL_STATE.smp_nearest      = sg_make_sampler(&(sg_sampler_desc){
		.label      = "sampler-nearest",
		.min_filter = SG_FILTER_NEAREST,
		.mag_filter = SG_FILTER_NEAREST,
	});
	SOKOL_STATE.smp_linear       = sg_make_sampler(&(sg_sampler_desc){
		.label      = "sampler-linear",
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
	});
	SOKOL_STATE.bind.samplers[0] = SOKOL_STATE.opts.video.filter == SYS_VIDEO_FILTER_NEAREST
		? SOKOL_STATE.smp_nearest
		: SOKOL_STATE.smp_linear;

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
#if MINI_GAMEPAD_ENABLE
	mg_gamepads_init(&SOKOL_STATE.gamepads);
#endif

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
			if(SOKOL_STATE.status == SOKOL_STATUS_INI) {
				sokol_pause();
			} else if(SOKOL_STATE.status == SOKOL_STATUS_PAUSED) {
				sokol_resume();
			}
		} break;
		case SAPP_KEYCODE_F6: {
			sokol_screenshot_save(SOKOL_STATE.frame_ctx.dst);
		} break;
		case SAPP_KEYCODE_F12: {
			marena_reset(&SOKOL_STATE.scratch_marena);
			struct alloc scratch = SOKOL_STATE.scratch;
			i32 w                = SYS_DISPLAY_W;
			i32 h                = SYS_DISPLAY_H;
			str8 dbgcmd          = str8_lit("ffmpeg -f rawvideo -pix_fmt rgba -s 400x240 -i frame.raw frame.png");
			FILE *test           = fopen("/tmp/frame.raw", "wb");
			ssize dst_size       = w * h * sizeof(u32);
			u32 *dst             = alloc_arr(scratch, dst, w * h);
			tex_opaque_to_rgba(SOKOL_STATE.frame_ctx.dst, dst, dst_size, SOKOL_STATE.opts.colors);
			fwrite(dst, sizeof(u32), w * h, test);
			fclose(test);
		} break;
		case SAPP_KEYCODE_F8: {
#if defined(SOKOL_RECORDING_ENABLED)
			struct recording_1b *rec = &SOKOL_STATE.recording;
			rec->idx                 = 0;
			rec->len                 = 0;
#endif
		} break;
		case SAPP_KEYCODE_F9: {
#if defined(SOKOL_RECORDING_ENABLED)
			sokol_recording_write(&SOKOL_STATE.recording);
#endif
		} break;
		case SAPP_KEYCODE_R: {
#if OS_MACOS
			if(ev->modifiers & SAPP_MODIFIER_SUPER) {
#else
			if(ev->modifiers & SAPP_MODIFIER_CTRL) {
#endif
				SOKOL_STATE.status = SOKOL_STATUS_RELOAD;
			}
		} break;
		case SAPP_KEYCODE_Q: {
#if OS_MACOS
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
		for(ssize i = 0; i < ev->num_touches; ++i) {
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
		for(ssize i = 0; i < ev->num_touches; ++i) {
			struct sapp_touchpoint touch = ev->touches[i];
			sokol_touch_remove(touch);
		}
	} break;
	default: {
	} break;
	}
	sokol_pause_handle_sokol_event(ev);
}

void
sokol_pause_handle_sokol_event(const sapp_event *ev)
{
	if(SOKOL_STATE.status != SOKOL_STATUS_PAUSED) { return; }

	i32 b = 0;
	switch(ev->type) {
	case SAPP_EVENTTYPE_KEY_DOWN: {
		switch(ev->key_code) {
		case SAPP_KEYCODE_W: {
			b |= SYS_INP_DPAD_U;
		} break;
		case SAPP_KEYCODE_S: {
			b |= SYS_INP_DPAD_D;
		} break;
		case SAPP_KEYCODE_A: {
			b |= SYS_INP_DPAD_L;
		} break;
		case SAPP_KEYCODE_D: {
			b |= SYS_INP_DPAD_R;
		} break;
		case SAPP_KEYCODE_PERIOD: {
			b |= SYS_INP_A;
		} break;
		case SAPP_KEYCODE_COMMA: {
			b |= SYS_INP_B;
		} break;
		case SAPP_KEYCODE_UP: {
			b |= SYS_INP_DPAD_U;
		} break;
		case SAPP_KEYCODE_DOWN: {
			b |= SYS_INP_DPAD_D;
		} break;
		case SAPP_KEYCODE_LEFT: {
			b |= SYS_INP_DPAD_L;
		} break;
		case SAPP_KEYCODE_RIGHT: {
			b |= SYS_INP_DPAD_R;
		} break;
		case SAPP_KEYCODE_X: {
			b |= SYS_INP_A;
		} break;
		case SAPP_KEYCODE_Z: {
			b |= SYS_INP_B;
		} break;
		case SAPP_KEYCODE_Q: {
			b |= SYS_INP_A;
		} break;
		case SAPP_KEYCODE_E: {
			b |= SYS_INP_B;
		} break;
		case SAPP_KEYCODE_SPACE: {
			b |= SYS_INP_A;
		} break;
		default: {
		} break;
		}
	} break;
	case SAPP_EVENTTYPE_KEY_UP: {
	} break;
	default: {
	} break;
	}
	sokol_pause_handle_buttons(b);
}

#if MINI_GAMEPAD_ENABLE
void
sokol_pause_handle_gamepad_event(const mg_event *ev)
{
	if(SOKOL_STATE.status != SOKOL_STATUS_PAUSED) { return; }

	i32 b = 0;
	switch(ev->type) {
	case MG_EVENT_BUTTON_PRESS: {
		switch(ev->button) {
		case MG_BUTTON_DPAD_UP: {
			b |= SYS_INP_DPAD_U;
		} break;
		case MG_BUTTON_DPAD_DOWN: {
			b |= SYS_INP_DPAD_D;
		} break;
		case MG_BUTTON_SOUTH: {
			b |= SYS_INP_A;
		} break;
		case MG_BUTTON_EAST: {
			b |= SYS_INP_B;
		} break;
		default: {
		} break;
		}
	} break;
	default: {
	} break;
	}
	sokol_pause_handle_buttons(b);
}
#endif

void
sokol_pause_handle_buttons(i32 buttons)
{
	struct sokol_menu *menu = &SOKOL_STATE.menu;
	b32 close               = false;

	if(menu->len > 0) {
		struct sokol_menu_item *item = menu->items + menu->idx;
		if(buttons & SYS_INP_A) {
			switch(item->type) {
			case SOKOL_MENU_ITEM_TYPE_ACTION: {
				close = true;
			} break;
			case SOKOL_MENU_ITEM_TYPE_BOOL: {
				if(item->type == SOKOL_MENU_ITEM_TYPE_BOOL) {
					item->value = !item->value;
				}
			} break;
			default: {
			} break;
			}
		}
		if(buttons & SYS_INP_DPAD_U) {
			menu->idx = max_i32(menu->idx - 1, 0);
		}
		if(buttons & SYS_INP_DPAD_D) {
			menu->idx = min_i32(menu->idx + 1, menu->len - 1);
		}
		if(buttons & SYS_INP_DPAD_R) {
			if(item->type == SOKOL_MENU_ITEM_TYPE_BOOL) {
				item->value = true;
			}
		}
		if(buttons & SYS_INP_DPAD_L) {
			if(item->type == SOKOL_MENU_ITEM_TYPE_BOOL) {
				item->value = false;
			}
		}
	} else {
		if((buttons & SYS_INP_A)) {
			close = true;
		}
	}

	if((buttons & SYS_INP_B)) {
		close = true;
	}

	if(close) {
		if(menu->len > 0) {
			struct sokol_menu_item *item = menu->items + menu->idx;
			if(item->callback) {
				item->callback(item->arg);
			}
		}
		sokol_resume();
	}
}

#define F32_SCALE (1.0f / I16_MAX)
void
sokol_stream_cb(f32 *buffer, int num_frames, int num_channels)
{
#if !defined(SOKOL_DISABLE_AUDIO)

	if(SOKOL_STATE.status == SOKOL_STATUS_INI) {
		dbg_assert(num_channels == SOKOL_AUDIO_CHANNEL_COUNT);
		// dbg_assert(num_frames == SOKOL_AUDIO_FRAMES);
		dbg_assert(num_frames < SOKOL_AUDIO_BUFFER_CAP);

		static i16 lbuf[SOKOL_AUDIO_BUFFER_CAP];
		static i16 rbuf[SOKOL_AUDIO_BUFFER_CAP];
		mclr_array(lbuf);
		mclr_array(rbuf);

		sys_internal_audio(lbuf, rbuf, num_frames);

		f32 *s     = buffer;
		i16 *l     = lbuf;
		i16 *r     = rbuf;
		f32 volume = SOKOL_AUDIO_VOLUME;

		for(i32 n = 0; n < num_frames; n++) {
			// Convert and apply volume for left channel
			f32 vl = (*l++ * F32_SCALE) * volume;
			f32 vr = vl;
			if(num_channels == 2) {
				vr = (*r++ * F32_SCALE) * volume;
			}

			// Store the f32 values in the output stream buffer
			*s++ = vl; // Left channel
			if(num_channels == 2) {
				*s++ = vr; // Right channel, only if stereo
			}
		}
	} else {
		mclr(buffer, num_frames * num_channels * sizeof(f32));
	}
#endif
}

void
sokol_frame(void)
{
	f32 win_w                       = sapp_widthf();
	f32 win_h                       = sapp_heightf();
	s_params_t params               = {.time = sys_time_elapsed()};
	s_buffer_params_t buffer_params = sokol_get_buffer_params(win_w, win_h);
	s_colors_t colors               = {0};
	usize size                      = ARRLEN(SOKOL_PIXELS);
#if MINI_GAMEPAD_ENABLE
	sokol_gamepads_ev();
#endif

	mcpy_struct(&colors.color_black, &COL_BLACK);
	mcpy_struct(&colors.color_white, &COL_WHITE);
	mcpy_struct(&colors.color_debug, &COL_RED);

	params.filter_mode           = SOKOL_STATE.opts.video.filter;
	SOKOL_STATE.bind.samplers[0] = SOKOL_STATE.opts.video.filter == SYS_VIDEO_FILTER_NEAREST
		? SOKOL_STATE.smp_nearest
		: SOKOL_STATE.smp_linear;

	// mcpy_array(colors.color_black, COL_PURPLE);
	// mcpy_array(colors.color_white, COL_PURPLE);
	// mcpy_array(colors.color_debug, COL_RED);

	tex_opaque_to_rgba(SOKOL_STATE.frame_ctx.dst, (u32 *)SOKOL_PIXELS, size, SOKOL_STATE.opts.colors);
	tex_opaque_to_rgba(SOKOL_STATE.debug_ctx.dst, (u32 *)SOKOL_PIXELS_DEBUG, size, SOKOL_STATE.opts.colors_dbg);

	sg_update_image(
		SOKOL_STATE.bind.images[IMG_tex],
		&(sg_image_data){
			.subimage[0][0] = {
				.ptr  = SOKOL_PIXELS,
				.size = size,
			},
		});

	sg_update_image(
		SOKOL_STATE.bind.images[IMG_tex_debug],
		&(sg_image_data){
			.subimage[0][0] = {
				.ptr  = SOKOL_PIXELS_DEBUG,
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

	if(SOKOL_STATE.status == SOKOL_STATUS_INI) {
		b32 updated = sys_internal_update();
		if(updated) {
#if defined(SOKOL_RECORDING_ENABLED)
			{
				struct alloc scratch     = SOKOL_STATE.scratch;
				struct recording_1b *rec = &SOKOL_STATE.recording;
				struct tex *src          = &SOKOL_STATE.frame_ctx.dst;
				struct tex *dst          = rec->frames + rec->idx;
				tex_cpy(dst, src);
				rec->idx = (rec->idx + 1) % rec->cap;
				rec->len = MIN(rec->len + 1, rec->cap);
			}
#endif
		}
	} else if(SOKOL_STATE.status == SOKOL_STATUS_PAUSED) {
		{
			struct gfx_ctx ctx = SOKOL_STATE.paused_state.ctx;
			tex_clr(ctx.dst, GFX_COL_BLACK);
			{
				struct tex tex     = SOKOL_STATE.paused_state.frame_tex;
				struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
				gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
				ctx.pat = gfx_pattern_50();
				gfx_rec_fill(ctx, 0, 0, tex.w, tex.h, PRIM_MODE_BLACK);
				ctx.pat = gfx_pattern_100();
			}
			{
				struct fnt fnt         = SOKOL_STATE.fnt;
				struct sokol_menu menu = SOKOL_STATE.menu;
				rec_i32 root           = {SYS_DISPLAY_W * 0.5f, 0, SYS_DISPLAY_W * 0.5f, SYS_DISPLAY_H};
				gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_BLACK);
				rec_i32_cut_left(&root, 3);
				gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_WHITE);

				{
					i32 menu_height = 99;
					rec_i32 layout  = rec_i32_cut_top(&root, menu_height);
					i32 row_height  = menu_height / 3;
					for(ssize i = 0; i < menu.len; ++i) {
						rec_i32 row_layout             = rec_i32_cut_top(&layout, row_height);
						struct sokol_menu_item item    = menu.items[i];
						str8 str                       = item.title;
						i32 value                      = item.value;
						enum sokol_menu_item_type type = item.type;
						rec_i32_cut_left(&row_layout, 10);
						rec_i32_cut_right(&row_layout, 10);
						v2_i32 cntr = rec_i32_cntr(row_layout);
						if(fnt.t.px != 0) {
							i32 x = row_layout.x + 4;
							i32 y = cntr.y - (fnt.cell_h * 0.5f);
							fnt_mono_draw_str(ctx, fnt, str, x, y, 0, 0, PRIM_MODE_BLACK);
						}
						switch(type) {
						case SOKOL_MENU_ITEM_TYPE_BOOL: {
							i32 margin      = 4;
							i32 checkbox_w  = 11;
							i32 checkbox_ww = checkbox_w * 0.5f;
							i32 x           = row_layout.x + row_layout.w - checkbox_w - margin;
							i32 y           = cntr.y - (checkbox_ww);
							gfx_rec_fill(ctx, x, y, checkbox_w, checkbox_w, PRIM_MODE_BLACK);
							if(value) {
								gfx_cir_fill(ctx, x + checkbox_ww, y + checkbox_ww, checkbox_w - 5, PRIM_MODE_WHITE);
							}
						} break;
						default: {
						} break;
						}
						if(menu.idx == i) {
							gfx_rec_fill(ctx, row_layout.x, cntr.y - 10, row_layout.w, 20, PRIM_MODE_INV);
						}
					}
				}
				{
					rec_i32 layout = rec_i32_cut_top(&root, 2);
					rec_i32_cut_left(&layout, 10);
					rec_i32_cut_right(&layout, 10);
					gfx_lin(ctx, layout.x, layout.y, layout.x + layout.w, layout.y, PRIM_MODE_BLACK);
					gfx_lin(ctx, layout.x, layout.y + 1, layout.x + layout.w, layout.y + 1, PRIM_MODE_BLACK);
				}
			}
			{
				struct tex tex     = SOKOL_STATE.paused_state.menu_tex;
				struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
				gfx_spr(ctx, src, -SOKOL_STATE.paused_state.x_offset, 0, 0, SPR_MODE_COPY);
			}
		}
		{
			struct gfx_ctx ctx = SOKOL_STATE.frame_ctx;
			struct tex tex     = SOKOL_STATE.paused_state.ctx.dst;
			struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
			gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
		}
	}

	if(SOKOL_STATE.status == SOKOL_STATUS_RELOAD) {
		sys_internal_init();
		SOKOL_STATE.status = SOKOL_STATUS_INI;
	}
}

void
sokol_cleanup(void)
{
	SOKOL_STATE.status = 0;
	sys_internal_close();
	sys_free(SOKOL_STATE.marena.buf);
	sg_shutdown();
#if !defined(SOKOL_DISABLE_AUDIO)
	saudio_shutdown();
#endif
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

#if MINI_GAMEPAD_ENABLE
	b |= sokol_gamepads_upd();
#endif

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

void
sys_1bit_invert(b32 i)
{
	dbg_not_implemeneted("sokol");

error:
	return;
}

v4
sys_color_v4_get(enum gfx_col color)
{
	v4 res = color_rgba_from_u32(SOKOL_STATE.opts.colors.colors[color]);
	return res;
}

void
sys_color_v4_set(enum gfx_col color, v4 value)
{
	SOKOL_STATE.opts.colors.colors[color] = color_rgba_to_u32(value);
}

u32
sys_color_u32_get(enum gfx_col color)
{
	u32 res = SOKOL_STATE.opts.colors.colors[color];
	return res;
}

void
sys_color_u32_set(enum gfx_col color, u32 value)
{
	SOKOL_STATE.opts.colors.colors[color] = value;
}

void *
sys_1bit_buffer(void)
{
	return SOKOL_STATE.frame_ctx.dst.px;
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

i32
sys_menu_item_add(
	const char *title,
	void (*callback)(void *arg),
	void *arg)
{
	dbg_assert(SOKOL_STATE.menu.len < (ssize)ARRLEN(SOKOL_STATE.menu.items));
	ssize idx                    = SOKOL_STATE.menu.len++;
	struct sokol_menu_item *item = SOKOL_STATE.menu.items + idx;
	item->id                     = SOKOL_STATE.menu.next_id++;
	item->type                   = SOKOL_MENU_ITEM_TYPE_ACTION;
	item->title                  = str8_cstr((char *)title);
	item->arg                    = arg;
	item->callback               = callback;
	return item->id;
}

i32
sys_menu_checkmark_add(const char *title, int val, void (*callback)(void *arg), void *arg)
{
	dbg_assert(SOKOL_STATE.menu.len < (ssize)ARRLEN(SOKOL_STATE.menu.items));
	ssize idx                    = SOKOL_STATE.menu.len++;
	struct sokol_menu_item *item = SOKOL_STATE.menu.items + idx;
	item->type                   = SOKOL_MENU_ITEM_TYPE_BOOL;
	item->title                  = str8_cstr((char *)title);
	item->arg                    = arg;
	item->callback               = callback;
	item->value                  = val;
	return item->id;
}

i32
sys_menu_options_add(const char *title, const char **options, int count, void (*callback)(void *arg), void *arg)
{
	dbg_assert(SOKOL_STATE.menu.len < (ssize)ARRLEN(SOKOL_STATE.menu.items));
	return 0;
}

int
sys_menu_value(int id)
{
	struct sokol_menu_item *items = SOKOL_STATE.menu.items;
	ssize len                     = SOKOL_STATE.menu.len;
	for(ssize i = 0; i < len; i++) {
		if(items[i].id == id) {
			return items[i].value;
		}
	}
	return 0;
}

void
sys_menu_item_remove(int id)
{
	struct sokol_menu_item *items = SOKOL_STATE.menu.items;
	ssize len                     = SOKOL_STATE.menu.len;

	// Find the index of the item with this id
	ssize idx = -1;
	for(ssize i = 0; i < len; i++) {
		if(items[i].id == id) {
			idx = i;
			break;
		}
	}

	// Not found → nothing to remove (or assert if you prefer)
	if(idx == -1) {
		return;
	}

	// Shift elements left to fill the gap
	for(ssize i = idx; i < len - 1; i++) {
		items[i] = items[i + 1];
	}

	// Clear last element (optional, for safety/debug)
	mclr_struct(&items[len - 1]);

	SOKOL_STATE.menu.len--;
}

void
sys_menu_clr(void)
{
	mclr_array(SOKOL_STATE.menu.items);
	SOKOL_STATE.menu.len = 0;
	SOKOL_STATE.menu.idx = 0;
}

void
sys_draw_debug_clear(void)
{
}

void
sys_debug_draw(struct debug_shape *shapes, int count)
{
#if BUILD_DEBUG
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

#if defined(SOKOL_RECORDING_ENABLED) && defined(SOKOL_DBG_AUDIO)
	struct recording_aud *rec = &SOKOL_STATE.recording_aud;
	for(ssize i = 0; i < rec->len; ++i) {
		i32 x = (f32)((f32)i / (f32)rec->cap) * SYS_DISPLAY_W;
		i32 y = (SYS_DISPLAY_H * 0.5f) + (rec->frames[i] * 1000.0f);
		gfx_cir(ctx, x, y, 1, 1);
	}
#endif

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

void
sokol_pause(void)
{
	SOKOL_STATE.status = SOKOL_STATUS_PAUSED;
	tex_cpy(&SOKOL_STATE.paused_state.frame_tex, &SOKOL_STATE.frame_ctx.dst);
	sys_internal_pause();
}

void
sokol_resume(void)
{
	SOKOL_STATE.status = SOKOL_STATUS_INI;
	sys_internal_resume();
}

void
sys_set_menu_image(struct tex tex, i32 x_offset)
{
	SOKOL_STATE.paused_state.x_offset = x_offset;
	if(tex.px == NULL) {
		tex_clr(SOKOL_STATE.paused_state.menu_tex, GFX_COL_CLEAR);
		return;
	}

	tex_clr(SOKOL_STATE.paused_state.menu_tex, GFX_COL_CLEAR);
	struct gfx_ctx ctx = gfx_ctx_default(SOKOL_STATE.paused_state.menu_tex);
	struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
	gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
}

int
sys_scores_queries_clear_queue(void)
{
	return 0;
}

int
sys_scores_mutations_clear_queue(void)
{
	return 0;
}

int
sys_score_add(str8 board_id, u32 value, sys_scores_req_callback callback, void *userdata)
{

	return 0;

	// error:
	// 	return -1;
}

int
sys_scores_get(str8 board_id, sys_scores_req_callback callback, void *userdata, struct alloc alloc)
{
	i32 res          = -1;
	u32 top_score    = U32_MAX;
	u32 bottom_score = 100000;

	static const str8 names[] = {
		str8_lit_comp("Gravinger"),
		str8_lit_comp("Clabbosaurus"),
		str8_lit_comp("Markantes"),
		str8_lit_comp("kartacha"),
		str8_lit_comp("7305899826608137"),
		str8_lit_comp("Dr.Bagels"),
		str8_lit_comp("msal"),
		str8_lit_comp("4461469380168897"),
		str8_lit_comp("GentleEel"),
		str8_lit_comp("NicBran98"),
		str8_lit_comp("loosecanons"),
		str8_lit_comp("RoliRoler"),
		str8_lit_comp("fenelope"),
		str8_lit_comp("yoyogigames"),
		str8_lit_comp("fp.monkey"),
		str8_lit_comp("Simply_In"),
	};

	i32 idx = rndm_range_i32(NULL, 0, (i32)(sizeof(names) / sizeof(names[0])) - 1);
	if(callback) {
		res                             = 0;
		i32 scores_count                = rndm_range_i32(NULL, 5, 10);
		u32 last_updated                = sys_epoch_2000(NULL);
		struct sys_scores_res score_res = {
			.type = SYS_SCORE_RES_SCORES_GET,
			.get  = {
				.board_id        = board_id,
				.last_updated    = last_updated,
				.player_included = true,
			},
		};
		struct sys_score_arr *entries = &score_res.get.entries;
		if(alloc.allocf != NULL) {
			entries->items = alloc_arr(alloc, entries->items, scores_count);
		}
		if(entries->items != NULL) {
			entries->cap = scores_count;
			entries->len = scores_count;

			for(ssize i = 0; i < scores_count; ++i) {
				str8 player;
				f32 t     = (f32)i / (f32)(scores_count - 1);
				f32 curve = t * t * t; /* quadratic falloff */
				u32 score = top_score - (u32)((top_score - bottom_score) * curve);
				score -= rndm_range_u32(NULL, 0, 20000u); /* small random jitter */

				if(i == 0) {
					player = str8_lit(SOKOL_MOCK_PLAYER_NAME);
				} else {
					i32 rndm_idx = rndm_range_i32(NULL, 0, ARRLEN(names) - 1);
					player       = (names[rndm_idx]);
				}

				entries->items[i] = (struct sys_score){
					.value  = score,
					.rank   = i > 8 ? rndm_range_i32(NULL, 11, 17000) : i,
					.player = player,
				};
			}
		}
		callback(0, score_res, userdata);
	}

	return res;
}

int
sys_scores_personal_best_get(str8 board_id, sys_scores_req_callback callback, void *userdata)
{
	i32 res                         = -1;
	struct sys_scores_res score_res = {
		.type          = SYS_SCORE_RES_SCORES_PERSONAL_BEST_GET,
		.personal_best = {
			.score = {
				.rank   = 0,
				.value  = U32_MAX,
				.player = str8_lit(SOKOL_MOCK_PLAYER_NAME),
			},
		},
	};

	if(callback) {
		res = 0;
		callback(0, score_res, userdata);
	}

	return res;
}

static inline b32
sokol_touch_add(sapp_touchpoint point, sapp_mousebutton button)
{
	b32 res = false;

	// Make sure the point doesn't already exist
	for(ssize i = 0; i < (ssize)ARRLEN(SOKOL_STATE.touches_mouse); ++i) {
		struct touch_point_mouse_emu emu = SOKOL_STATE.touches_mouse[i];
		dbg_check(emu.id != point.identifier, "sokol", "Touch point already exists %" PRIxPTR "", emu.id);
	}

	for(ssize i = 0; i < (ssize)ARRLEN(SOKOL_STATE.touches_mouse); ++i) {
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
	for(ssize i = 0; i < (ssize)ARRLEN(SOKOL_STATE.touches_mouse); ++i) {
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
	struct alloc scratch     = SOKOL_STATE.scratch;
	str8 icons_dir           = sokol_path_to_res_path(str8_lit("icons"));
	tinydir_dir *dir         = alloc_struct(scratch, dir);
	str8 png                 = str8_lit(".png");
	sapp_icon_desc icon_desc = {.sokol_default = true};

	tinydir_open(dir, (char *)icons_dir.str);
	log_info("sokol", "loading icons from: %s", icons_dir.str);

	i32 icon_count = 0;
	while(dir->has_next) {
		tinydir_file file;
		tinydir_readfile(dir, &file);
		tinydir_next(dir);

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
	tinydir_close(dir);

	if(icon_count > 0) {
		icon_desc.sokol_default = false;
		sapp_set_icon(&icon_desc);
	}

	for(ssize i = 0; i < icon_count; ++i) {
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
	if(SOKOL_STATE.opts.video.scaling == SYS_VIDEO_SCALING_INTEGER) {
		scale = floor_f32(scale);
	} else if(SOKOL_STATE.opts.video.scaling == SYS_VIDEO_SCALING_OVERSCALE) {
		scale = ceil_f32(scale);
	}
	scale        = max_f32(scale, 1.0f);
	res.scale.x  = scale;
	res.scale.y  = scale;
	res.size.x   = res.app_size.x * res.scale.x;
	res.size.y   = res.app_size.y * res.scale.y;
	res.offset.x = (res.win_size.x - res.size.x) * 0.5f;
	res.offset.y = (res.win_size.y - res.size.y) * 0.5f;
	if(SOKOL_STATE.opts.video.scaling == SYS_VIDEO_SCALING_INTEGER ||
		SOKOL_STATE.opts.video.scaling == SYS_VIDEO_SCALING_OVERSCALE) {
		res.offset.x = floor_f32(res.offset.x);
		res.offset.y = floor_f32(res.offset.y);
	}

	dbg_assert(res.scale.x != 0.0f);
	dbg_assert(res.scale.y != 0.0f);
	return res;
}

void
sys_set_app_name(str8 value)
{
	sapp_set_window_title((const char *)value.str);
}

#define SOKOL_SCREENSHOT_FORMAT 1

static void
sokol_screenshot_save(struct tex tex)
{
	marena_reset(&SOKOL_STATE.scratch_marena);
	static u32 data[SYS_DISPLAY_W * SYS_DISPLAY_H] = {0};
	usize size                                     = ARRLEN(data);
	struct alloc alloc                             = SOKOL_STATE.scratch;
	struct date_time date_time                     = date_time_from_epoch_2000_gmt(sys_epoch_2000(NULL));
	i32 w                                          = SYS_DISPLAY_W;
	i32 h                                          = SYS_DISPLAY_H;
	i32 comp                                       = 4;
	i32 stride_in_bytes                            = w * comp;

	tex_opaque_to_rgba(tex, data, size, SOKOL_STATE.opts.screentshot.colors);
	str8 path = str8_fmt_push(alloc,
		"%.*s/%s-%04d-%02d-%02d_%02d:%02d:%02d",
		(int)SOKOL_STATE.opts.screentshot.save_path.size,
		SOKOL_STATE.opts.screentshot.save_path.str,
		SOKOL_NAME,
		date_time.year,
		date_time.month,
		date_time.day,
		date_time.hour,
		date_time.min,
		date_time.sec);

#if SOKOL_SCREENSHOT_FORMAT == 1
	path = str8_fmt_push(alloc, "%s.png", path.str);
	stbi_write_png((char *)path.str, w, h, comp, data, stride_in_bytes);
#else
	path = str8_fmt_push(alloc, "%s.bmp", path.str);
	stbi_write_bmp((char *)path.str, w, h, comp, data);
#endif
	log_info("sokol", "screentshot saved: %s", path.str);
}

// https://github.com/tsoding/rendering-video-in-c-with-ffmpeg/blob/master/ffmpeg_linux.c
static void
sokol_recording_write(struct recording_1b *recording)
{
	if(!recording || recording->len == 0) return;
	marena_reset(&SOKOL_STATE.scratch_marena);

	struct alloc scratch = SOKOL_STATE.scratch;
	int w                = recording->frames[0].w;
	int h                = recording->frames[0].h;

	FILE *pipe = NULL;

	// Generate timestamped output path
	struct date_time dt = date_time_from_epoch_2000_gmt(sys_epoch_2000(NULL));
	str8 path           = str8_fmt_push(
		scratch,
		"%.*s/%s-%04d-%02d-%02d_%02d:%02d:%02d.mp4",
		(int)SOKOL_STATE.opts.recording.save_path.size,
		SOKOL_STATE.opts.recording.save_path.str,
		SOKOL_NAME,
		dt.year,
		dt.month,
		dt.day,
		dt.hour,
		dt.min,
		dt.sec);

	// Construct ffmpeg command
	i32 fps                   = sys_ups_target_get();
	i32 scale                 = SOKOL_STATE.opts.recording.scale;
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
		tex_opaque_to_rgba(src, dst, dst_size, SOKOL_STATE.opts.recording.colors);
		fwrite(dst, sizeof(u32), w * h, pipe);
	}

error:;
	if(pipe) {
		pclose(pipe);
	}
}

#if MINI_GAMEPAD_ENABLE
static inline i32
sokol_gamepads_upd(void)
{
	i32 res                      = 0;
	struct mg_gamepads *gamepads = &SOKOL_STATE.gamepads;
	mg_gamepads_poll(gamepads);

	mg_gamepad *gamepad = gamepads->list.head;

	if(!gamepad) { goto end; }

	for(ssize i = 0; i < MG_BUTTON_COUNT; i++) {
		mg_button button_type  = i;
		mg_button_state button = gamepad->buttons[i];
		if(button.supported == MG_FALSE) continue;
		if(button.current == MG_FALSE) continue;

		switch(button_type) {
		case MG_BUTTON_SOUTH: {
			res |= SYS_INP_A;
		} break;
		case MG_BUTTON_EAST: {
			res |= SYS_INP_B;
		} break;
		case MG_BUTTON_WEST: {
			res |= SYS_INP_A;
		} break;
		case MG_BUTTON_NORTH: {
			res |= SYS_INP_B;
		} break;
		case MG_BUTTON_LEFT_SHOULDER: {
			res |= SYS_INP_B;
		} break;
		case MG_BUTTON_RIGHT_SHOULDER: {
			res |= SYS_INP_A;
		} break;
		case MG_BUTTON_DPAD_LEFT: {
			res |= SYS_INP_DPAD_L;
		} break;
		case MG_BUTTON_DPAD_RIGHT: {
			res |= SYS_INP_DPAD_R;
		} break;
		case MG_BUTTON_DPAD_UP: {
			res |= SYS_INP_DPAD_U;
		} break;
		case MG_BUTTON_DPAD_DOWN: {
			res |= SYS_INP_DPAD_D;
		} break;
		default: {
		} break;
		}
	}

	for(ssize i = 0; i < MG_AXIS_COUNT; i++) {
		mg_axis axis_type  = i;
		mg_axis_state axis = gamepad->axes[i];
		f32 value          = axis.value;
		switch(axis_type) {
		case MG_AXIS_LEFT_X: {
			if(value > 0.8f) {
				res |= SYS_INP_DPAD_R;
			}
			if(value < -0.8f) {
				res |= SYS_INP_DPAD_L;
			}
		} break;
		case MG_AXIS_LEFT_Y: {
			if(value > 0.8f) {
				res |= SYS_INP_DPAD_D;
			}
			if(value < -0.8f) {
				res |= SYS_INP_DPAD_U;
			}
		} break;
		case MG_AXIS_LEFT_TRIGGER: {
			if(value > 0.8f) {
				res |= SYS_INP_B;
			}
		} break;
		case MG_AXIS_RIGHT_TRIGGER: {
			if(value > 0.8f) {
				res |= SYS_INP_A;
			}
		} break;
		default: {
		} break;
		}
	}

end:;
	return res;
}

static inline void
sokol_gamepads_ev(void)
{
	i32 res                      = 0;
	struct mg_gamepads *gamepads = &SOKOL_STATE.gamepads;
	mg_gamepads_poll(gamepads);
	mg_gamepad *gamepad = gamepads->list.head;

	if(!gamepad) { goto end; }

	mg_event ev;
	while(mg_gamepads_check_event(gamepads, &ev)) {
		sokol_pause_handle_gamepad_event(&ev);
		switch(ev.type) {
		case MG_EVENT_BUTTON_PRESS: {
			switch(ev.button) {
			case MG_BUTTON_BACK: {
				if(SOKOL_STATE.status == SOKOL_STATUS_INI) {
					sokol_pause();
				} else if(SOKOL_STATE.status == SOKOL_STATUS_PAUSED) {
					sokol_resume();
				}
			} break;
			case MG_BUTTON_START: {
				if(SOKOL_STATE.status == SOKOL_STATUS_INI) {
					sokol_pause();
				} else if(SOKOL_STATE.status == SOKOL_STATUS_PAUSED) {
					sokol_resume();
				}
			} break;
			}
		} break;
		case MG_EVENT_BUTTON_RELEASE: {
			switch(ev.button) {
			case MG_BUTTON_BACK: {
			} break;
			case MG_BUTTON_GUIDE: {
			} break;
			}
		} break;
		default: {
		} break;
		}
	}

end:;
}
#endif
