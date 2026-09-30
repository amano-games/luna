#pragma once

#include "base/types.h"
#include "engine/gfx/gfx-defs.h"

static const u32 APP_DEFAULT_WHITE = 0xA5A5A2FF;
static const u32 APP_DEFAULT_BLACK = 0x110B0DFF;

static const u32 GFX_1B_PALETTES[][GFX_COL_NUM_COUNT] = {
	{APP_DEFAULT_BLACK, APP_DEFAULT_WHITE}, // Default
	{0x000000FF, 0xFFFFFFFF},               // Black & White
	{0x0F380FFF, 0x9BBC0FFF},               // Game Boy
	{0x1D0F44FF, 0xF44E38FF}                // Sunset

};

static const u32 COL_U32_WHITE    = 0xffffffff;
static const u32 COL_U32_BLACK    = 0x000000ff;
static const f32 COL_V3_WHITE[3]  = {0.64f, 0.64f, 0.64f};
static const f32 COL_V3_BLACK[3]  = {0.05f, 0.04f, 0.06f};
static const f32 COL_V3_RED[3]    = {1.0f, 0.0f, 0.0f};
static const f32 COL_V3_YELLOW[3] = {1.0f, 0.784f, 0.2f};
static const f32 COL_V3_PURPLE[3] = {0.424f, 0.0f, 1.0f};
