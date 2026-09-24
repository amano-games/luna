#include "sys/sys-img.h"
#include "base/dbg.h"
#include "base/str.h"
#include "sys/sys-io.h"

#define SYS_IMG_LOG      "sys-img"
#define SYS_IMG_PNG_COMP 4

#if OS_PLAYDATE
#include "sys/playdate/sys-playdate.h"
#define STBIW_MALLOC(sz)        PD->system->realloc(NULL, (sz))
#define STBIW_REALLOC(p, newsz) PD->system->realloc((p), (newsz))
#define STBIW_FREE(p)           ((void)PD->system->realloc((p), 0))
#define STBI_WRITE_NO_STDIO
#define STBIW_ASSERT(x) dbg_assert(x)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static void
sys_img_stbi_w(void *ctx, void *data, int size)
{
	sys_file *f = (sys_file *)ctx;
	sys_file_w(*f, data, (u32)size);
}

b32
sys_img_write(struct tex tex, str8 path, struct gfx_col_pallete pallete, struct alloc scratch)
{
	b32 res    = false;
	sys_file f = sys_file_zero();
	u32 *rgba  = NULL;
	i32 w      = tex.w;
	i32 h      = tex.h;
	i32 stride = w * SYS_IMG_PNG_COMP;
	ssize px_n = (ssize)w * (ssize)h;

	dbg_check(tex.fmt == TEX_FMT_1B_OPAQUE || tex.fmt == TEX_FMT_8B_INDEX, SYS_IMG_LOG, "tex must be opaque");
	dbg_check(tex.px1b != NULL, SYS_IMG_LOG, "tex has no pixels");
	dbg_check(w > 0 && h > 0, SYS_IMG_LOG, "invalid tex size %d x %d", w, h);

	rgba = alloc_arr(scratch, rgba, px_n);
	dbg_check_mem(rgba, SYS_IMG_LOG);
	tex_opaque_to_rgba(tex, rgba, px_n, pallete);

	f = sys_file_open_w(path);
	dbg_check(sys_file_is_valid(f), SYS_IMG_LOG, "failed to open %.*s", str8_spread(path));

	dbg_check(
		stbi_write_png_to_func(sys_img_stbi_w, &f, w, h, SYS_IMG_PNG_COMP, rgba, stride),
		SYS_IMG_LOG,
		"failed to write png %.*s",
		str8_spread(path));

	res = true;

error:
	if(sys_file_is_valid(f)) {
		sys_file_close(f);
	}
	return res;
}
