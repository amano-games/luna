#include "sys-sokol.h"
#include "base/mathfunc.h"
#include "base/marena.h"
#include "base/mem.h"
#include "lib/tex/tex.h"
#include "sys-debug-draw.h"
#include "base/types.h"
#include <stdio.h>
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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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
// #define SOKOL_PIXEL_PERFECT
#if defined(TARGET_WIN)
// #define SOKOL_DISABLE_AUDIO
#endif

// #define SOKOL_DBG_AUDIO
// #define SOKOL_AUDIO_FRAMES        256
#define SOKOL_AUDIO_CHANNEL_COUNT 1
#define SOKOL_AUDIO_VOLUME        0.1f
#define SOKOL_AUDIO_BUFFER_CAP    0x1000

#define SOKOL_RECORDING_SECONDS 120
#define SOKOL_RECORDING_SCALE   3
#define SOKOL_RECORDING_ENABLED

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

static const str8 STEAM_RUNTIME_RELATIVE_PATH = str8_lit_comp("steam-runtime");

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

struct sys_opts {
	struct gfx_col_pallete colors;
	struct gfx_col_pallete colors_dbg;
	struct sys_screenshot_opts screentshot;
	struct sys_recording_opts recording;
};

struct sokol_state {
	i32 state;

	struct marena scratch_marena;
	struct alloc scratch;

	struct marena marena;
	struct alloc alloc;

	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;

	f32 mouse_scroll_sensitivity;

	struct gfx_ctx frame_ctx;
	struct gfx_ctx debug_ctx;

	u8 keys[SYS_KEYS_LEN];
	b32 crank_docked;
	f32 crank;
	f32 volume;

	f32 mouse_x;
	f32 mouse_y;
	u32 mouse_btns;

	struct sys_opts opts;

	struct recording_1b recording;
	struct recording_aud recording_aud;
	struct touch_point_mouse_emu touches_mouse[SAPP_MAX_TOUCHPOINTS];

	struct sys_process_info process_info;
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

static void sokol_process_info_set(void);
static void sokol_set_icon(void);
static inline void sokol_tex_to_rgba(const u8 *in, u32 *out, usize size, const u32 *pal);
static inline b32 sokol_touch_add(sapp_touchpoint point, sapp_mousebutton button);
static inline b32 sokol_touch_remove(sapp_touchpoint point);
static void sokol_screenshot_save(struct tex tex);
static void sokol_recording_write(struct recording_1b *recording);
str8 sokol_path_to_res_path(struct str8 path);
static inline s_buffer_params_t sokol_get_buffer_params(f32 win_w, f32 win_h);

sapp_desc
sokol_main(i32 argc, char **argv)
{
	stm_setup();
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
	sokol_process_info_set();

	struct str8 exe_path = SOKOL_STATE.process_info.exe_path;
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

	{
		// Ini opts
		SOKOL_STATE.opts.recording.seconds_count = SOKOL_RECORDING_SECONDS;
		SOKOL_STATE.opts.recording.scale         = SOKOL_RECORDING_SCALE;
	}

	{
		struct tex tex        = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, SOKOL_STATE.alloc);
		SOKOL_STATE.frame_ctx = gfx_ctx_default(tex);
		tex_clr(tex, GFX_COL_BLACK);
		dbg_check(tex.px, "sokol", "Failed to create frame buffer");
	}

	{
		struct tex tex        = tex_create_opaque(SYS_DISPLAY_W, SYS_DISPLAY_H, SOKOL_STATE.alloc);
		SOKOL_STATE.debug_ctx = gfx_ctx_default(tex);
		tex_clr(tex, GFX_COL_BLACK);
		dbg_check(tex.px, "sokol", "Failed to create debug buffer");
	}

#if defined(SOKOL_RECORDING_ENABLED)
	{
		struct recording_1b *rec = &SOKOL_STATE.recording;
		struct alloc alloc       = SOKOL_STATE.alloc;
		rec->cap                 = SYS_UPS * SOKOL_STATE.opts.recording.seconds_count;
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
		rec->cap                  = SYS_UPS * SOKOL_STATE.opts.recording.seconds_count;
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
		SOKOL_STATE.opts.colors.colors[GFX_COL_BLACK] = 0x110B0DFF;
		SOKOL_STATE.opts.colors.colors[GFX_COL_WHITE] = 0xA5A5A2FF;

		SOKOL_STATE.opts.colors_dbg.colors[GFX_COL_BLACK] = 0x000000FF;
		SOKOL_STATE.opts.colors_dbg.colors[GFX_COL_WHITE] = 0xFFFFFFFF;

		SOKOL_STATE.opts.recording.colors.colors[GFX_COL_BLACK] = 0x000000FF;
		SOKOL_STATE.opts.recording.colors.colors[GFX_COL_WHITE] = 0xFFFFFFFF;

		SOKOL_STATE.opts.screentshot.colors.colors[GFX_COL_BLACK] = 0x000000FF;
		SOKOL_STATE.opts.screentshot.colors.colors[GFX_COL_WHITE] = 0xFFFFFFFF;
	}

	{
		str8 dir_path = sys_path_to_data_path(
			SOKOL_STATE.scratch,
			str8_lit(""),
			str8_lit(SOKOL_ORG),
			str8_lit(SOKOL_NAME));
		sys_make_dir(dir_path);
	}

	SOKOL_STATE.state = 1;

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
}

#define F32_SCALE (1.0f / I16_MAX)
void
sokol_stream_cb(f32 *buffer, int num_frames, int num_channels)
{
#if !defined(SOKOL_DISABLE_AUDIO)
	if(SOKOL_STATE.state == 1) {
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
	s_params_t params               = {.time = sys_seconds()};
	s_buffer_params_t buffer_params = sokol_get_buffer_params(win_w, win_h);
	s_colors_t colors               = {0};
	usize size                      = ARRLEN(SOKOL_PIXELS);

	mcpy_struct(&colors.color_black, &COL_BLACK);
	mcpy_struct(&colors.color_white, &COL_WHITE);
	mcpy_struct(&colors.color_debug, &COL_RED);

#if defined SOKOL_PIXEL_PERFECT
	params.pixel_perfect = true;
#else
	params.pixel_perfect = false;
#endif

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
}

void
sokol_cleanup(void)
{
	SOKOL_STATE.state = 0;
	sys_internal_close();
	sys_free(SOKOL_STATE.marena.buf);
	sg_shutdown();
#if !defined(SOKOL_DISABLE_AUDIO)
	saudio_shutdown();
#endif
}

struct str8
sys_base_path(void)
{
	return SOKOL_STATE.process_info.base_path;
}

str8
sys_exe_path(void)
{
	return SOKOL_STATE.process_info.exe_path;
}

// https://wiki.libsdl.org/SDL3/SDL_GetPrefPath
str8
sys_data_path(void)
{
	return SOKOL_STATE.process_info.data_path;
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
#if !defined(TARGET_WIN)
#include <time.h>
#include <sys/time.h>
#endif
// TODO: Win32 support
// Returns seconds since 2000-01-01 UTC.
// If milliseconds != NULL, stores the 0–999 ms remainder.
u32
sys_epoch_2000(u32 *milliseconds)
{
#if !defined(TARGET_WIN)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	u64 unix_seconds = (u64)ts.tv_sec;
	u64 seconds      = unix_seconds - SECONDS_BETWEEN_1970_AND_2000;

	if(milliseconds) {
		*milliseconds = (u32)(ts.tv_nsec / 1000000ULL); // 0–999 ms
	}

	return seconds;
#else
	return 0;
#endif
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
	return SOKOL_STATE.frame_ctx.dst.px;
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
sys_alloc(void *ptr, ssize size, ssize align)
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
	void *res = (void *)fopen((char *)path.str, "wb");
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

ssize
sys_file_w(void *f, const void *buf, u32 buf_size)
{
	i32 count = 1;
	ssize res = fwrite(buf, buf_size, count, (FILE *)f);
	return res;
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

#if defined(TARGET_WIN)
#include <stdlib.h>
#include <direct.h>
#endif
b32
sys_make_dir(str8 path)
{
	b32 res = false;
	marena_reset(&SOKOL_STATE.scratch_marena);
	struct alloc scratch = SOKOL_STATE.scratch;

#if defined(TARGET_LINUX) || defined(TARGET_MACOS)
	{
		str8 path_copy = str8_cpy_push(scratch, path);
		if(mkdir((char *)path_copy.str, 0755) != -1) {
			res = 1;
		}
	}
#endif

#if defined(TARGET_WIN)
	_mkdir((char *)path.str);
#endif

#if defined(TARGET_WIN) && 0
	str16 name16                         = str16_from_8(scratch, path);
	WIN32_FILE_ATTRIBUTE_DATA attributes = {0};
	GetFileAttributesExW((WCHAR *)name16.str, GetFileExInfoStandard, &attributes);
	if(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		result = 1;
	} else if(CreateDirectoryW((WCHAR *)name16.str, 0)) {
		result = 1;
	}
#endif

	return res;
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
#if defined(DEBUG)
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

usize
sys_file_modified(str8 path)
{
	// 	dbg_not_implemeneted("sokol");
	//
	// error:
	return 0;
}

static inline void
sokol_tex_to_rgba(const u8 *in, u32 *out, usize size, const u32 *pal)
{
	u32 *pixels = out;
	for(i32 y = 0; y < SYS_DISPLAY_H; y++) {
		for(i32 x = 0; x < SYS_DISPLAY_W; x++) {
			i32 src     = (x >> 3) + y * SYS_DISPLAY_WBYTES;
			i32 dst     = x + y * SYS_DISPLAY_W;
			i32 byt     = in[src];
			i32 bit     = !!(byt & 0x80 >> (x & 7));
			pixels[dst] = pal[!bit] | 0xFF000000;
		}
	}
}

void
sys_set_menu_image(struct tex tex, i32 x_offset)
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
sokol_process_info_set(void)
{
	marena_reset(&SOKOL_STATE.scratch_marena);
	SOKOL_STATE.process_info      = (struct sys_process_info){0};
	struct alloc alloc            = SOKOL_STATE.alloc;
	struct alloc scratch          = SOKOL_STATE.scratch;
	struct sys_process_info *info = &SOKOL_STATE.process_info;

#if !defined(TARGET_WASM)
	{
		// Exe PATH
		ssize str_size = wai_getExecutablePath(NULL, 0, NULL);
		if(str_size > 0) {
			u8 *path = (u8 *)alloc_arr(scratch, path, str_size);
			wai_getExecutablePath((char *)path, str_size, NULL);
			info->exe_path = str8_cpy_push(alloc, (str8){.str = (u8 *)path, .size = str_size});
		}
	}
#endif

#if !defined(TARGET_WASM)
	{
		// Module PATH
		ssize str_size = wai_getModulePath(NULL, 0, NULL);

		if(str_size > 0) {
			u8 *path = alloc_arr(scratch, path, str_size);
			wai_getModulePath((char *)path, str_size, NULL);
			info->module_path = str8_cpy_push(alloc, (str8){.str = (u8 *)path, .size = str_size});
		}
	}
#endif

	{
		// Initial path
		info->initial_path = sys_get_current_path(alloc);
	}

	{
		// Data Path
#if defined(TARGET_LINUX)
		{
			// TODO: Fallback?
			char *xdg      = getenv("XDG_DATA_HOME");
			char *home     = getenv("HOME");
			str8 data_path = str8_lit("");
			if(xdg != NULL) {
				data_path = str8_cstr(xdg);
			} else if(home != NULL) {
				data_path = str8_cstr(home);
			}
			info->data_path = str8_cpy_push(alloc, data_path);
		}
#endif

#if defined(TARGET_WIN)
		{
			ssize mem_size = MKILOBYTE(32);
			u16 *buffer    = alloc_arr(scratch, buffer, mem_size);
			// TODO: split os layer in windows/linux/mac so I can inclide windows headers in a single file
			// TODO: Support for strings u16 for windows things
#if 0
			if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR *)buffer))) {
				info->data_path = str8_from_16(arena, str16_cstring(buffer));
			}
#endif
		}
#endif

#if defined(TARGET_MACOS)
		{
			str8 home       = str8_cstr(getenv("HOME"));
			str8 suffix     = str8_lit("/Library/Application Support");
			info->data_path = str8_cat_push(alloc, home, suffix);
		}
#endif
	}

	{
		// Base path
#if defined(TARGET_MACOS)
		{
			str8 exe_path = SOKOL_STATE.process_info.exe_path;
			if(exe_path.size > 0) {
				str8 macos                 = str8_chop_last_slash(exe_path);
				str8 contents              = str8_chop_last_slash(macos);
				str8 resources_rel         = str8_lit("Resources");
				enum path_style path_style = path_style_from_str8(resources_rel);
				struct str8_list path_list = {0};
				str8_list_push(scratch, &path_list, contents);
				str8_list_push(scratch, &path_list, resources_rel);
				str8 resources_path                = path_join_by_style(alloc, &path_list, path_style);
				SOKOL_STATE.process_info.base_path = resources_path;
			}
		}
#endif
	}
}

struct sys_process_info
sys_process_info(void)
{
	return SOKOL_STATE.process_info;
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

str8
sys_get_current_path(struct alloc alloc)
{
	str8 res = {0};
	marena_reset(&SOKOL_STATE.scratch_marena);
	struct alloc scratch = SOKOL_STATE.scratch;

#if defined(TARGET_LINUX) || defined(TARGET_MACOS)
	{
		char *cwdir = getcwd(0, 0);
		res         = str8_cpy_push(alloc, str8_cstr(cwdir));
		free(cwdir);
	}
#endif

#if defined(TARGET_WIN) && 0
	{
		DWORD length = GetCurrentDirectoryW(0, 0);
		u16 *memory  = alloc_arr(scratch, memory, length + 1);
		length       = GetCurrentDirectoryW(length + 1, (WCHAR *)memory);
		res          = str8_from_16(alloc, str16(memory, length));
	}
#endif

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
	tinydir_close(&dir);

	if(icon_count > 0) {
		icon_desc.sokol_default = false;
		sapp_set_icon(&icon_desc);
	}

	for(ssize i = 0; i < icon_count; ++i) {
		stbi_image_free((char *)icon_desc.images[i].pixels.ptr);
	}
}

str8
sys_path_to_data_path(struct alloc alloc, struct str8 path, str8 org_name, str8 app_name)
{
	str8 res       = path;
	str8 data_path = sys_data_path();
	if(data_path.size == 0) { return res; }

	/*
    On Windows, the string might look like:
    C:\\Users\\bob\\AppData\\Roaming\\My Company\\My Program Name\\
    On Linux, the string might look like:
    /home/bob/.local/share/My Program Name/
    On Mac OS X, the string might look like:
    /Users/bob/Library/Application Support/My Program Name/
  */

	// TODO: support windows
	enum path_style path_style = path_style_from_str8(path);
	struct str8_list path_list = {0};
	str8_list_push(alloc, &path_list, data_path);
	str8_list_push(alloc, &path_list, app_name);
	str8_list_push(alloc, &path_list, path);
	res = path_join_by_style(alloc, &path_list, path_style);

	return res;
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
		"%s-%04d-%02d-%02d_%02d:%02d:%02d",
		SOKOL_NAME,
		date_time.year,
		date_time.month,
		date_time.day,
		date_time.hour,
		date_time.min,
		date_time.sec);
	path      = sys_path_to_data_path(alloc, path, str8_lit(SOKOL_ORG), str8_lit(SOKOL_NAME));

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
        "%s-%04d-%02d-%02d_%02d:%02d:%02d.mp4",
        SOKOL_NAME,
        dt.year,
        dt.month,
        dt.day,
        dt.hour,
        dt.min,
        dt.sec);
	path = sys_path_to_data_path(
		scratch,
		path,
		str8_lit(SOKOL_ORG),
		str8_lit(SOKOL_NAME));

	// Construct ffmpeg command
	i32 fps                   = SYS_UPS;
	i32 scale                 = SOKOL_STATE.opts.recording.scale;
	struct str8_list cmd_list = {0};
	str8_list_pushf(scratch, &cmd_list, "ffmpeg");
#if defined(DEBUG)
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
