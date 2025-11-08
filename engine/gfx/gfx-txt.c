#include "gfx-txt.h"
#include "base/mathfunc.h"
#include "engine/gfx/gfx-spr.h"
#include "base/dbg.h"
#include "base/trace.h"

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
	TRACE_START(__func__);
	dbg_assert(fnt.cell_h > 0);
	dbg_assert(fnt.cell_w > 0);
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
			dbg_assert(ci > 31);
			t.r.x = ((ci - 32) % fnt.grid_w) * fnt.cell_w;
			t.r.y = ((ci - 32) / fnt.grid_w) * fnt.cell_h;
			gfx_spr(ctx, t, p.x, p.y, 0, mode);
			i32 move_x = fnt_char_size_x_px(fnt, ci, cbi, tracking);
			p.x += move_x;
		}
	}
	TRACE_END();
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
	TRACE_START(__func__);
	rec_i32 res = {0};
	if(fnt.t.px != NULL) {
		v2_i32 text_size = fnt_size_px(fnt, str, tracking, leading);
		i32 txt_x        = x - (i32)(text_size.x * pivot.x);
		i32 txt_y        = y - (i32)(text_size.y * pivot.y);
		fnt_draw_str(ctx, fnt, str, txt_x, txt_y, tracking, leading, mode);
		res = (rec_i32){txt_x, txt_y, text_size.x, text_size.y};
	}
	TRACE_END();
	return res;
}

rec_i32
fnt_draw_str_block(
	struct gfx_ctx ctx,
	struct fnt fnt,
	struct str8_list lines,
	rec_i32 layout,
	i32 tracking,
	i32 leading,
	enum spr_mode mode,
	u32 align_flags)
{
	rec_i32 res      = {0};
	i32 max_width    = 0;
	i32 total_height = 0;

	if(lines.node_count == 0) return res;

	// Measure all lines
	for(struct str8_node *n = lines.first; n; n = n->next) {
		v2_i32 size = fnt_size_px(fnt, n->str, tracking, leading);
		max_width   = max_i32(max_width, size.x);
	}
	total_height = lines.node_count * fnt.cell_h + (lines.node_count - 1) * leading;

	// Compute top-left of text block inside layout rect
	i32 block_x = layout.x;
	i32 block_y = layout.y;

	// Horizontal alignment (within layout.w)
	if(align_flags & FNT_ALIGN_H_CENTER) {
		block_x += (layout.w - max_width) * 0.5f;
	} else if(align_flags & FNT_ALIGN_H_END) {
		block_x += (layout.w - max_width);
	}

	// Vertical alignment (within layout.h)
	if(align_flags & FNT_ALIGN_V_CENTER) {
		block_y += (layout.h - total_height) * 0.5f;
	} else if(align_flags & FNT_ALIGN_V_END) {
		block_y += (layout.h - total_height);
	}

	// Draw and track extents
	i32 min_x = INT32_MAX;
	i32 max_x = INT32_MIN;
	i32 min_y = block_y;
	i32 max_y = block_y + total_height;

	i32 line_y = block_y;

	for(struct str8_node *n = lines.first; n; n = n->next) {
		str8 line        = n->str;
		v2_i32 line_size = fnt_size_px(fnt, line, tracking, leading);
		i32 line_width   = line_size.x;

		i32 line_x = block_x;

		// Optional per-line internal alignment (if desired)
		if(align_flags & FNT_ALIGN_H_CENTER) {
			line_x = layout.x + (layout.w - line_width) / 2;
		} else if(align_flags & FNT_ALIGN_H_END) {
			line_x = layout.x + layout.w - line_width;
		}

		min_x = min_i32(min_x, line_x);
		max_x = max_i32(max_x, line_x + line_width);

		fnt_draw_str(ctx, fnt, line, line_x, line_y, tracking, leading, mode);
		line_y += fnt.cell_h + leading;
	}

	res.x = min_x;
	res.y = min_y;
	res.w = max_x - min_x;
	res.h = max_y - min_y;

	return res;
}
