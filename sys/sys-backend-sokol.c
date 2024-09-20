#if !defined(TARGET_WASM)
#include <whereami.h>
#endif

#include "sys-assert.h"
#include "sys-backend.h"
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
#include "shaders/sokol_shader.h"

static struct {
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
	u8 frame_buffer[SYS_DISPLAY_WBYTES * SYS_DISPLAY_H];
	u8 keys[350];
	u8 update_row[SYS_DISPLAY_H];
	bool32 crank_docked;
	f32 crank;
} STATE;

void
event(const sapp_event *e)
{
	if(e->type == SAPP_EVENTTYPE_KEY_DOWN) {
		STATE.keys[e->key_code] = 1;
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
		STATE.keys[e->key_code] = 0;
	} else if(e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
		STATE.crank_docked = true;
		STATE.crank += e->scroll_y * -0.03f;
		STATE.crank = fmodf(STATE.crank, 1.0f);
	}
}

void
init(void)
{
	STATE.crank_docked = true;
	stm_setup();
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	/* a pass action to framebuffer to black */
	STATE.pass_action = (sg_pass_action){
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {0.25f, 0.5f, 0.75f, 1.0f}},
	};

	STATE.bind.fs.samplers[SLOT_smp] = sg_make_sampler(&(sg_sampler_desc){
		.label      = "sampler",
		.min_filter = SG_FILTER_NEAREST,
		.mag_filter = SG_FILTER_NEAREST,
	});

	STATE.bind.fs.images[SLOT_tex] = sg_make_image(
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

	STATE.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
		.data  = SG_RANGE(vertices),
		.label = "quad-vertices",
	});

	STATE.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
		.type  = SG_BUFFERTYPE_INDEXBUFFER,
		.data  = SG_RANGE(indices),
		.label = "quad-indices",
	});

	sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

	STATE.pip = sg_make_pipeline(&(sg_pipeline_desc){
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
	sys_init();
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
			// if(!STATE.update_row[y]) continue;
			// STATE.update_row[y] = 0;
			for(i32 x = 0; x < SYS_DISPLAY_W; x++) {
				i32 i     = (x >> 3) + y * SYS_DISPLAY_WBYTES;
				i32 k     = x + y * SYS_DISPLAY_W;
				i32 byt   = STATE.frame_buffer[i];
				i32 bit   = !!(byt & 0x80 >> (x & 7));
				pixels[k] = pal[!bit];
			}
		}
		sg_update_image(
			STATE.bind.fs.images[SLOT_tex],
			&(sg_image_data){
				.subimage[0][0] = {
					.ptr  = pixels,
					.size = SYS_DISPLAY_W * SYS_DISPLAY_H * 4},
			});
	}

	sg_begin_pass(&(sg_pass){.action = STATE.pass_action, .swapchain = sglue_swapchain()});
	sg_apply_pipeline(STATE.pip);
	sg_apply_bindings(&STATE.bind);
	sg_draw(0, 6, 1);
	sg_end_pass();
	sg_commit();
	sys_tick(NULL);
}

void
cleanup(void)
{
	sg_shutdown();
	sys_close();
}

static const char *STEAM_RUNTIME_RELATIVE_PATH = "steam-runtime";
sapp_desc
sokol_main(i32 argc, char **argv)
{

	if(!getenv("STEAM_RUNTIME")) {
		struct exe_path exe_path = backend_where();
		if(exe_path.path.len != 0) {

			i32 runtime_path_size = 1 + exe_path.path.len + strlen(STEAM_RUNTIME_RELATIVE_PATH) + 1 /* for the nul byte */;
			char *runtime_path    = (char *)malloc(runtime_path_size);
			stbsp_snprintf(runtime_path, runtime_path_size, "%.*s/%s", (i32)exe_path.path.len, exe_path.path.data, STEAM_RUNTIME_RELATIVE_PATH);

			log_info("SYS", "dirname:  %.*s", (i32)exe_path.path.len, exe_path.path.data);
			log_info("SYS", "basename:  %.*s", (i32)exe_path.dirname.len, exe_path.dirname.data);
			log_info("SYS", "STEAM_RUNTIME %s", runtime_path);

			setenv("STEAM_RUNTIME", runtime_path, 1);
			free(exe_path.path.data);
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

i32
backend_inp(void)
{
	i32 b    = 0;
	u8 *keys = STATE.keys;

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

struct exe_path
backend_where(void)
{
	struct exe_path res = {0};
	i32 len             = 0;
	i32 dirname_len     = 0;
	char *path          = NULL;
#if !defined(TARGET_WASM)
	len = wai_getExecutablePath(NULL, 0, &dirname_len);
#endif

	if(len > 0) {
		path = (char *)malloc(len + 1);
#if !defined(TARGET_WASM)
		wai_getExecutablePath(path, len, &dirname_len);
#endif
		path[len]         = '\0';
		path[dirname_len] = '\0';

		res.path.str     = path;
		res.path.size    = len;
		res.dirname.str  = path + dirname_len + 1;
		res.dirname.size = len - dirname_len - 1;
	}

	return res;
}

// TODO: Translate playdate keys to SOKOL keys
i32
backend_key(i32 key)
{
	u8 *keys = STATE.keys;
	return keys[key];
}

u8 *
backend_keys(void)
{
	return STATE.keys;
}

f32
backend_crank(void)
{
	return STATE.crank;
}

i32
backend_crank_docked(void)
{
	return STATE.crank_docked;
}

void
backend_display_row_updated(i32 a, i32 b)
{
	assert(0 <= a && b < SYS_DISPLAY_H);
	for(i32 i = a; i <= b; i++) {
		STATE.update_row[i] = 1;
	}
}

f32
backend_seconds(void)
{
	return stm_sec(stm_since(0));
}

u32 *
backend_framebuffer(void)
{
	return (u32 *)STATE.frame_buffer;
}

void *
backend_alloc(void *ptr, usize size)
{
	return malloc(size);
}

void
backend_free(void *ptr)
{
	free(ptr);
}

void
backend_log(const char *tag, u32 log_level, u32 log_item, const char *msg, u32 line_nr, const char *filename)
{
	slog_func(tag, log_level, log_item, msg, line_nr, filename, NULL);
}

long
get_file_size(const char *path)
{
	FILE *fp = backend_file_open(path, SYS_FILE_R);

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
backend_file_stats(str8 path)
{
	// TODO: Fill the other stats
	struct sys_file_stats res = {0};
	int size                  = get_file_size(path.str);
	if(size < 0) {
		log_error("IO", "failed to get file stats %s", path.str);
	}
	res.size = size;
	return res;
}

void *
backend_file_open(const char *path, i32 mode)
{
	switch(mode) {
	case SYS_FILE_R: {
		void *res = (void *)fopen(path, "rb");
		return res;
	}
	case SYS_FILE_W: {
		void *res = (void *)fopen(path, "w");
		return res;
	}
	case SYS_FILE_A: {
		void *res = (void *)fopen(path, "ab");
		return res;
	}
	default:
		NOT_IMPLEMENTED;
		return NULL;
	}
}

i32
backend_file_close(void *f)
{
	return fclose((FILE *)f);
}

i32
backend_file_flush(void *f)
{
	return fflush((FILE *)f);
}

i32
backend_file_read(void *f, void *buf, usize buf_size)
{
	i32 count = 1;
	usize s   = fread(buf, buf_size, count, (FILE *)f);
	if(s == 0) {
		log_error("IO", "Error reading from file: %d", (int)s);
	}

	return (i32)s;
}

i32
backend_file_write(void *f, const void *buf, usize buf_size)
{
	int count = 1;
	usize s   = fwrite(buf, buf_size, count, (FILE *)f);
	return (i32)s;
}

i32
backend_file_tell(void *f)
{
	usize t = ftell((FILE *)f);
	return (i32)t;
}

i32
backend_file_seek(void *f, i32 pos, i32 origin)
{
	// TODO: not sure
	return fseek((FILE *)f, pos, origin);
}

i32
backend_file_remove(const char *path)
{
	NOT_IMPLEMENTED;
	return 0;
}

void *
backend_menu_item_add(const char *title, void (*callback)(void *arg), void *arg)
{
	return NULL;
}

void *
backend_menu_checkmark_add(const char *title, i32 val, void (*callback)(void *arg), void *arg)
{
	return NULL;
}

void *
backend_menu_options_add(const char *title, const char **options, i32 count, void (*callback)(void *arg), void *arg)
{
	return NULL;
}

i32
backend_menu_value(void *ptr)
{
	return 0;
}

void
backend_menu_clr(void)
{
}

i32
backend_debug_space(void)
{
	return 0;
}

void
backend_draw_debug_clear(void)
{
}

void
backend_draw_debug_shape(struct debug_shape *shapes, i32 count)
{
}
