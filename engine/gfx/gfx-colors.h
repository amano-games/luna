#pragma once

#include "base/types.h"
#include "engine/gfx/gfx-defs.h"

#define APP_DEFAULT_WHITE 0xA5A5A2FFu
#define APP_DEFAULT_BLACK 0x110B0DFFu

static const u32 GFX_1B_PALETTES[][GFX_COL_NUM_COUNT] = {
	{[GFX_COL_BLACK] = APP_DEFAULT_BLACK, [GFX_COL_WHITE] = APP_DEFAULT_WHITE}, // Default
	{[GFX_COL_BLACK] = 0x000000FF, [GFX_COL_WHITE] = 0xFFFFFFFF},               // Black & White
	{[GFX_COL_BLACK] = 0x0F380FFF, [GFX_COL_WHITE] = 0x9BBC0FFF},               // Game Boy
	{[GFX_COL_BLACK] = 0x1D0F44FF, [GFX_COL_WHITE] = 0xF44E38FF},               // Sunset
	{[GFX_COL_BLACK] = 0x12101FFF, [GFX_COL_WHITE] = 0x8672FFFF},               // Purple Night
	{[GFX_COL_BLACK] = 0x4C3118FF, [GFX_COL_WHITE] = 0xD7A564FF},               // Parchment
	{[GFX_COL_BLACK] = 0x6F09B7FF, [GFX_COL_WHITE] = 0x83DD47FF},               // TMNT
	{[GFX_COL_BLACK] = 0x004D58FF, [GFX_COL_WHITE] = 0x99D7FFFF},               // Aqua
	{[GFX_COL_BLACK] = 0x5F112EFF, [GFX_COL_WHITE] = 0xFF4848FF},               // VB
	{[GFX_COL_BLACK] = 0x7E8280FF, [GFX_COL_WHITE] = 0xDBE1DCFF},               // Low Contrast
	{[GFX_COL_BLACK] = 0xEF289DFF, [GFX_COL_WHITE] = 0xFFE673FF},               // MBorg
	{[GFX_COL_BLACK] = 0x000000FF, [GFX_COL_WHITE] = 0xFFE673FF},               // Aurus
	{[GFX_COL_BLACK] = 0x000000FF, [GFX_COL_WHITE] = 0xFF2C80FF},               // Pinky
	{[GFX_COL_BLACK] = 0x000000FF, [GFX_COL_WHITE] = 0xFF6D4BFF},               // Orengi
	{[GFX_COL_BLACK] = 0x000000FF, [GFX_COL_WHITE] = 0x72dec2FF},               // Merv
	{[GFX_COL_BLACK] = 0x290973FF, [GFX_COL_WHITE] = 0x00FFBDFF},               // Tensie

};

static const u32 COL_U32_WHITE    = 0xffffffff;
static const u32 COL_U32_BLACK    = 0x000000ff;
static const f32 COL_V3_WHITE[3]  = {0.64f, 0.64f, 0.64f};
static const f32 COL_V3_BLACK[3]  = {0.05f, 0.04f, 0.06f};
static const f32 COL_V3_RED[3]    = {1.0f, 0.0f, 0.0f};
static const f32 COL_V3_YELLOW[3] = {1.0f, 0.784f, 0.2f};
static const f32 COL_V3_PURPLE[3] = {0.424f, 0.0f, 1.0f};
