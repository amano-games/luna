#include "gfx-txt.h"
#include "assets/fnt.h"
#include "gfx/gfx-spr.h"

void
fnt_draw_str(
	struct gfx_ctx ctx,
	struct fnt fnt,
	i32 x,
	i32 y,
	str8 str,
	i32 tracking,
	i32 leading,
	i32 mode)
{
	v2_i32 p         = (v2_i32){x, y};
	struct tex_rec t = {0};
	t.t              = fnt.t;
	t.r.w            = fnt.cell_w;
	t.r.h            = fnt.cell_h;
	for(usize n = 0; n < str.size; n++) {
		i32 ci         = str.str[n];
		i32 cbi        = (n < str.size - 1) ? str.str[n + 1] : -1;
		i32 is_newline = (ci == '\n');
		p.x            = is_newline ? x : p.x;
		p.y += is_newline * (fnt.cell_h + leading);

		if(!is_newline) {
			assert(ci > 31);
			t.r.x = ((ci - 32) % fnt.grid_w) * fnt.cell_w;
			t.r.y = ((ci - 32) / fnt.grid_w) * fnt.cell_h;
			gfx_spr(ctx, t, p.x, p.y, 0, mode);
			i32 move_x = fnt_char_size_x_px(fnt, ci, cbi, tracking);
			p.x += move_x;
		}
	}
}
