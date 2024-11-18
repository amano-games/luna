#include "sys-sokol.h"

#if !defined(TARGET_WASM)
#include <whereami.h>
#endif

#include "sys-assert.h"
#include "sys-debug.h"
#include "sys-input.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys.h"

#define STB_IMAGE_IMPLEMENTATION

#define SOKOL_IMPL
#define SOKOL_ASSERT(c) assert(c);

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_time.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_audio.h"
#include "shaders/sokol_shader.h"

struct sokol_state {
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
	f32 mouse_scroll_sensitivity;
	u8 frame_buffer[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];
	u8 keys[SYS_KEYS_LEN];
	bool32 crank_docked;
	f32 crank;
};

static struct sokol_state SOKOL_STATE;

void
event(const sapp_event *e)
{
	if(e->type == SAPP_EVENTTYPE_KEY_DOWN) {
		SOKOL_STATE.keys[e->key_code] = 1;
		switch(e->key_code) {
		case SAPP_KEYCODE_ESCAPE: {
			sapp_request_quit();
		} break;
		case SAPP_KEYCODE_R: {
			if(e->modifiers & SAPP_MODIFIER_CTRL) {
				// sys_close();
				// sys_init();
			}
		} break;
		default: {
		} break;
		}

	} else if(e->type == SAPP_EVENTTYPE_KEY_UP) {
		SOKOL_STATE.keys[e->key_code] = 0;
	} else if(e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
		SOKOL_STATE.crank_docked = false;
		SOKOL_STATE.crank += e->scroll_y * -SOKOL_STATE.mouse_scroll_sensitivity;
		SOKOL_STATE.crank = fmodf(SOKOL_STATE.crank, 1.0f);
	}
}

#define F32_SCALE (1.0f / I16_MAX)
static void
stream_cb(f32 *buffer, int num_frames, int num_channels)
{
	assert(1 == num_channels);
	bool32 is_mono = (num_channels == 1);

	static i16 lbuf[0x1000];
	static i16 rbuf[0x1000];
	mclr(lbuf, sizeof(lbuf));
	mclr(rbuf, sizeof(rbuf));

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
init(void)
{
	SOKOL_STATE.crank_docked             = true;
	SOKOL_STATE.mouse_scroll_sensitivity = 0.03f;
	stm_setup();
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	saudio_setup(&(saudio_desc){
		// .stream_cb   = stream_cb,
		.stream_cb   = stream_cb,
		.logger.func = slog_func,
	});

	/* a pass action to framebuffer to black */
	SOKOL_STATE.pass_action = (sg_pass_action){
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {0.25f, 0.5f, 0.75f, 1.0f}},
	};

	SOKOL_STATE.bind.fs.samplers[SLOT_smp] = sg_make_sampler(&(sg_sampler_desc){
		.label      = "sampler",
		.min_filter = SG_FILTER_NEAREST,
		.mag_filter = SG_FILTER_NEAREST,
	});

	SOKOL_STATE.bind.fs.images[SLOT_tex] = sg_make_image(
		&(sg_image_desc){
			.width        = SYS_DISPLAY_W,
			.height       = SYS_DISPLAY_H,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
			.usage        = SG_USAGE_STREAM,
		});

	// clang-format off
    const float vertices[] = {
        // pos          // uv
        -1.0f,  1.0f,   0.0, 1.0,
         1.0f,  1.0f,   1.0, 1.0,
         1.0f, -1.0f,   1.0, 0.0,
        -1.0f, -1.0f,   0.0, 0.0,
    };
	// We need 2 triangles for a square, this makes 6 indexes.
	u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };
	// clang-format on

	SOKOL_STATE.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.data  = SG_RANGE(vertices),
		.label = "quad-vertices",
	});

	SOKOL_STATE.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
		.type  = SG_BUFFERTYPE_INDEXBUFFER,
		.data  = SG_RANGE(indices),
		.label = "quad-indices",
	});

	sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

	SOKOL_STATE.pip = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = shd,
		// If the vertex layout doesn't have gaps, there is no need to provide strides and offsets.
		.layout = {
			.attrs = {
				[ATTR_vs_pos].format       = SG_VERTEXFORMAT_FLOAT2,
				[ATTR_vs_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
			}},
		.label      = "pipeline",
		.index_type = SG_INDEXTYPE_UINT16,
		.cull_mode  = SG_CULLMODE_NONE,
	});

	sapp_show_mouse(false);
	sys_internal_init();
}

void
frame(void)
{
	const u32 pal[2] = {
		0xA2A5A5,
		0x0D0B11,
	};
	u32 *pixels_ptr[SYS_DISPLAY_W * SYS_DISPLAY_H * 4] = {0};
	{
		u32 *pixels = (u32 *)pixels_ptr;
		for(i32 y = 0; y < SYS_DISPLAY_H; y++) {
			// TODO: figure out dirty rows
			// if(!SOKOL_STATE.update_row[y]) continue;
			// SOKOL_STATE.update_row[y] = 0;
			for(i32 x = 0; x < SYS_DISPLAY_W; x++) {
				i32 i     = (x >> 3) + y * SYS_DISPLAY_WBYTES;
				i32 k     = x + y * SYS_DISPLAY_W;
				i32 byt   = SOKOL_STATE.frame_buffer[i];
				i32 bit   = !!(byt & 0x80 >> (x & 7));
				pixels[k] = pal[!bit];
			}
		}
		sg_update_image(
			SOKOL_STATE.bind.fs.images[SLOT_tex],
			&(sg_image_data){
				.subimage[0][0] = {
					.ptr  = pixels,
					.size = SYS_DISPLAY_W * SYS_DISPLAY_H * 4},
			});
	}

	sg_begin_pass(&(sg_pass){.action = SOKOL_STATE.pass_action, .swapchain = sglue_swapchain()});
	sg_apply_pipeline(SOKOL_STATE.pip);
	sg_apply_bindings(&SOKOL_STATE.bind);
	sg_draw(0, 6, 1);
	sg_end_pass();
	sg_commit();
	sys_internal_update();
}

void
cleanup(void)
{
	sg_shutdown();
	saudio_shutdown();
	sys_internal_close();
}

static const char *STEAM_RUNTIME_RELATIVE_PATH = "steam-runtime";
sapp_desc
sokol_main(i32 argc, char **argv)
{

	if(!getenv("STEAM_RUNTIME")) {
		struct exe_path exe_path = sys_sokol_where();
		if(exe_path.path.size != 0) {

			i32 runtime_path_size = 1 + exe_path.path.size + strlen(STEAM_RUNTIME_RELATIVE_PATH) + 1 /* for the nul byte */;
			char *runtime_path    = (char *)malloc(runtime_path_size);
			stbsp_snprintf(runtime_path, runtime_path_size, "%.*s/%s", (i32)exe_path.path.size, exe_path.path.str, STEAM_RUNTIME_RELATIVE_PATH);

			log_info("SYS", "dirname:  %.*s", (i32)exe_path.path.size, exe_path.path.str);
			log_info("SYS", "basename:  %.*s", (i32)exe_path.dirname.size, exe_path.dirname.str);
			log_info("SYS", "STEAM_RUNTIME %s", runtime_path);

			setenv("STEAM_RUNTIME", runtime_path, 1);
			free(exe_path.path.str);
		}
	}

	return (sapp_desc){
		.width              = SYS_DISPLAY_W * 2,
		.height             = SYS_DISPLAY_H * 2,
		.init_cb            = init,
		.frame_cb           = frame,
		.cleanup_cb         = cleanup,
		.event_cb           = event,
		.logger.func        = slog_func,
		.icon.sokol_default = true,
		.window_title       = "Devils on the Moon Pinball",

	};
}

struct exe_path
sys_sokol_where(void)
{
	struct exe_path res = {0};
	i32 dirname_len     = 0;
	str8 path           = {0};
#if !defined(TARGET_WASM)
	path.size = wai_getExecutablePath(NULL, 0, &dirname_len);
#endif

	if(path.size > 0) {
		path.str = (u8 *)malloc(path.size + 1);
#if !defined(TARGET_WASM)
		wai_getExecutablePath((char *)path.str, path.size, &dirname_len);
#endif
		path.str[path.size]   = '\0';
		path.str[dirname_len] = '\0';

		res.path         = path;
		res.dirname.str  = path.str + dirname_len + 1;
		res.dirname.size = path.size - dirname_len - 1;
	}

	return res;
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
sys_seconds(void)
{
	return stm_sec(stm_since(0));
}

// TODO: Make sure it works
u32
sys_epoch(u32 *milliseconds)
{
	u64 epoch = 1730405055;
	return stm_sec(stm_since(epoch));
}

void
sys_1bit_invert(bool32 i)
{
	NOT_IMPLEMENTED;
}

void *
sys_1bit_buffer(void)
{
	return (u32 *)SOKOL_STATE.frame_buffer;
}

void *
sys_alloc(void *ptr, usize size)
{
	return malloc(size);
}

void
sys_free(void *ptr)
{
	free(ptr);
}

void
sys_log(const char *tag, u32 log_level, u32 log_item, const char *msg, u32 line_nr, const char *filename)
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

bool32
sys_file_del(str8 path)
{
	return 0;
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
	return 1.0f;
}

void
sys_audio_lock(void)
{
}

void
sys_audio_unlock(void)
{
}
