#pragma once

#include "sys-types.h"
#include "mem.h"

#define GFX_PATTERN_NUM 17
#define GFX_PATTERN_MAX (GFX_PATTERN_NUM - 1)

enum {
	TEX_FMT_OPAQUE, // only color pixels
	TEX_FMT_MASK,   // color and mask interlaced in words
};

enum gfx_col {
	GFX_COL_BLACK,
	GFX_COL_WHITE,
	GFX_COL_CLEAR,
};

enum spr_mode {          // pattern holes always transparent
	SPR_MODE_COPY,       // 0 kDrawModeCopy
	SPR_MODE_WHITE_ONLY, // 1 kDrawModeBlackTransparent
	SPR_MODE_BLACK_ONLY, // 2 kDrawModeWhiteTransparent
	SPR_MODE_BLACK,      // 3 kDrawModeFillBlack
	SPR_MODE_WHITE,      // 4 kDrawModeFillWhite
	SPR_MODE_NXOR,       // 5 kDrawModeNXOR
	SPR_MODE_XOR,        // 6 kDrawModeXOR
	SPR_MODE_INV,        // 7 kDrawModeInverted
};

enum prim_mode {
	PRIM_MODE_BLACK,       // fills black, pattern holes are transparent
	PRIM_MODE_WHITE,       // fills white, pattern holes are transparent
	PRIM_MODE_WHITE_BLACK, // fills black, pattern holes are white
	PRIM_MODE_BLACK_WHITE, // fills white, pattern holes are black
	PRIM_MODE_INV,         // inverts canvas, pattern holes are transparent
};

enum spr_flip {
	SPR_FLIP_X = 1, // kBitmapFlippedX
	SPR_FLIP_Y = 2, // kBitmapFlippedY
};

struct tex {
	u32 *px; // either black/white words or black/white and transparent/opaque words interlaced
	int wword;
	int fmt;
	int w;
	int h;
};

struct tex_rec {
	struct tex t;
	rec_i32 r;
};

struct gfx_pattern {
	u32 p[8];
};

struct gfx_ctx {
	struct tex dst;
	struct gfx_pattern pat;
	i32 clip_x1;
	i32 clip_x2;
	i32 clip_y1;
	i32 clip_y2;
};

#define gfx_pattern_100()   gfx_pattern_bayer_4x4(16)
#define gfx_pattern_75()    gfx_pattern_bayer_4x4(12)
#define gfx_pattern_50()    gfx_pattern_bayer_4x4(8)
#define gfx_pattern_25()    gfx_pattern_bayer_4x4(4)
#define gfx_pattern_0()     gfx_pattern_bayer_4x4(0)
#define gfx_pattern_white() gfx_pattern_100()
#define gfx_pattern_black() gfx_pattern_0()

struct tex tex_frame_buffer(void);
struct tex tex_create(i32 w, i32 h, struct alloc alloc);
struct tex tex_create_opaque(i32 w, i32 h, struct alloc alloc);
struct tex tex_load(str8 path, struct alloc alloc);

void tex_clr(struct tex dst, i32 col);

i32 tex_px_at(struct tex tex, i32 x, i32 y);
i32 tex_mask_at(struct tex tex, i32 x, i32 y);
void tex_px(struct tex tex, i32 x, i32 y, i32 col);
void tex_mask(struct tex tex, i32 x, i32 y, i32 col);

struct gfx_pattern gfx_pattern_2x2(i32 p0, i32 p1);
struct gfx_pattern gfx_pattern_4x4(i32 p0, i32 p1, i32 p2, i32 p3);
struct gfx_pattern gfx_pattern_8x8(i32 p0, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5, i32 p6, i32 p7);
struct gfx_pattern gfx_pattern_bayer_4x4(i32 i);
struct gfx_pattern gfx_pattern_interpolate(i32 num, i32 den);
struct gfx_pattern gfx_pattern_interpolatec(i32 num, i32 den, i32 (*ease)(i32 a, i32 b, i32 num, i32 den));

void gfx_rec(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h, i32 r, enum prim_mode mode);
void gfx_rec_fill(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h, enum prim_mode mode);
void gfx_cir(struct gfx_ctx ctx, i32 px, i32 py, i32 d, enum prim_mode mode);
void gfx_cir_fill(struct gfx_ctx ctx, i32 px, i32 py, i32 d, enum prim_mode mode);
void gfx_fill_rows(struct tex dst, struct gfx_pattern pat, i32 y1, i32 y2);
void gfx_lin(struct gfx_ctx ctx, i32 ax, i32 ay, i32 bx, i32 by, enum prim_mode mode);
void gfx_lin_thick(struct gfx_ctx ctx, i32 ax, i32 ay, i32 bx, i32 by, i32 r, enum prim_mode mode);
void gfx_arc(struct gfx_ctx ctx, i32 px, i32 py, f32 sa, f32 ea, i32 d, enum prim_mode mode);
void gfx_arc_thick(struct gfx_ctx ctx, i32 px, i32 py, f32 sa, f32 ea, i32 d, i32 thick, enum prim_mode mode);
void gfx_poly(struct gfx_ctx ctx, v2_i32 *verts, i32 count, i32 r, enum prim_mode mode);

struct gfx_ctx gfx_ctx_display(void);
struct gfx_ctx gfx_ctx_unclip(struct gfx_ctx ctx);
struct gfx_ctx gfx_ctx_clip(struct gfx_ctx ctx, i32 x1, i32 y1, i32 x2, i32 y2);
struct gfx_ctx gfx_ctx_clip_top(struct gfx_ctx ctx, i32 y1);
struct gfx_ctx gfx_ctx_clip_bot(struct gfx_ctx ctx, i32 y2);
struct gfx_ctx gfx_ctx_clip_left(struct gfx_ctx ctx, i32 x1);
struct gfx_ctx gfx_ctx_clip_right(struct gfx_ctx ctx, i32 x2);
struct gfx_ctx gfx_ctx_clipr(struct gfx_ctx ctx, rec_i32 r);
struct gfx_ctx gfx_ctx_clipwh(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h);
