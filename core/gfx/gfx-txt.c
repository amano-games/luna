#include "gfx-txt.h"
#include "gfx/gfx-spr.h"
#include "dbg.h"

void
fnt_draw_str(
	struct gfx_ctx ctx,
	struct fnt fnt,
	str8 str,
	i32 x,
	i32 y,
	i32 tracking,
	i32 leading,
	i32 mode)
{
	assert(fnt.cell_h > 0);
	assert(fnt.cell_w > 0);
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

rec_i32
fnt_draw_str_pivot(
	struct gfx_ctx ctx,
	struct fnt fnt,
	str8 str,
	i32 x,
	i32 y,
	i32 tracking,
	i32 leading,
	v2 pivot,
	enum spr_mode mode)
{
	if(fnt.t.px != NULL) {
		v2_i32 text_size = fnt_size_px(fnt, str, tracking, leading);
		i32 txt_x        = x - (i32)(text_size.x * pivot.x);
		i32 txt_y        = y - (i32)(text_size.y * pivot.y);
		fnt_draw_str(ctx, fnt, str, txt_x, txt_y, tracking, leading, mode);
		return (rec_i32){txt_x, txt_y, text_size.x, text_size.y};
	}
	return (rec_i32){0};
}
