#pragma once

#include "base/mathfunc.h"
#include "base/str.h"
#include "base/types.h"
#include "base/utils.h"

u32
color_rgba_to_u32(v4 rgba)
{
	u32 result = 0;
	result |= ((u32)((u8)(rgba.x * 255.f))) << 24;
	result |= ((u32)((u8)(rgba.y * 255.f))) << 16;
	result |= ((u32)((u8)(rgba.z * 255.f))) << 8;
	result |= ((u32)((u8)(rgba.w * 255.f))) << 0;
	return result;
}

v4
color_rgba_from_u32(u32 hex)
{
	v4 result = {
		.x = ((hex & 0xff000000) >> 24) / 255.f,
		.y = ((hex & 0x00ff0000) >> 16) / 255.f,
		.z = ((hex & 0x0000ff00) >> 8) / 255.f,
		.w = ((hex & 0x000000ff) >> 0) / 255.f,
	};
	return result;
}

str8
color_rgba_to_hex_str(struct alloc alloc, v4 rgba)
{
	str8 hex_string = str8_fmt_push(alloc, "%02x%02x%02x%02x", (u8)(rgba.x * 255.f), (u8)(rgba.y * 255.f), (u8)(rgba.z * 255.f), (u8)(rgba.w * 255.f));
	return hex_string;
}

v4
color_rgba_from_hex_str(str8 hex_string)
{
	u8 byte_text[8]   = {0};
	u64 byte_text_idx = 0;
	for(u64 idx = 0; idx < hex_string.size && byte_text_idx < ARRLEN(byte_text); idx += 1) {
		if(char_is_digit(hex_string.str[idx], 16)) {
			byte_text[byte_text_idx] = char_to_lower(hex_string.str[idx]);
			byte_text_idx += 1;
		}
	}
	u8 byte_vals[4] = {0};
	for(u64 idx = 0; idx < 4; idx += 1) {
		byte_vals[idx] = (u8)str8_to_u64((str8){.str = &byte_text[idx * 2], .size = 2}, 16);
	}
	v4 rgba = {.x = byte_vals[0] / 255.f, .y = byte_vals[1] / 255.f, .z = byte_vals[2] / 255.f, .w = byte_vals[3] / 255.f};
	return rgba;
}

// hsv <-> rgb
static inline v3
color_hsv_from_rgb(v3 rgb)
{
	f32 c_max = max_f32(rgb.x, max_f32(rgb.y, rgb.z));
	f32 c_min = min_f32(rgb.x, min_f32(rgb.y, rgb.z));
	f32 delta = c_max - c_min;
	f32 h     = ((delta == 0.f) ? 0.f : (c_max == rgb.x) ? mod_f32((rgb.y - rgb.z) / delta + 6.f, 6.f)
				: (c_max == rgb.y)                       ? (rgb.z - rgb.x) / delta + 2.f
				: (c_max == rgb.z)                       ? (rgb.x - rgb.y) / delta + 4.f
														 : 0.f);
	f32 s     = (c_max == 0.f) ? 0.f : (delta / c_max);
	f32 v     = c_max;
	v3 hsv    = {.x = h / 6.f, .y = s, .z = v};
	return hsv;
}

static inline v3
color_rgb_from_hsv(v3 hsv)
{
	f32 h = mod_euc_f32(hsv.x * 360.f, 360.f);
	f32 s = hsv.y;
	f32 v = hsv.z;

	f32 c = v * s;
	f32 x = c * (1.f - abs_f32(mod_f32(h / 60.f, 2.f) - 1.f));
	f32 m = v - c;

	f32 r = 0;
	f32 g = 0;
	f32 b = 0;

	if((h >= 0.f && h < 60.f) || (h >= 360.f && h < 420.f)) {
		r = c;
		g = x;
		b = 0;
	} else if(h >= 60.f && h < 120.f) {
		r = x;
		g = c;
		b = 0;
	} else if(h >= 120.f && h < 180.f) {
		r = 0;
		g = c;
		b = x;
	} else if(h >= 180.f && h < 240.f) {
		r = 0;
		g = x;
		b = c;
	} else if(h >= 240.f && h < 300.f) {
		r = x;
		g = 0;
		b = c;
	} else if((h >= 300.f && h <= 360.f) || (h >= -60.f && h <= 0.f)) {
		r = c;
		g = 0;
		b = x;
	}

	v3 rgb = {.x = r + m, .y = g + m, .z = b + m};
	return (rgb);
}

static inline v4
color_hsva_from_rgba(v4 rgba)
{
	v3 rgb  = {.x = rgba.x, .y = rgba.y, .z = rgba.z};
	v3 hsv  = color_hsv_from_rgb(rgb);
	v4 hsva = {.x = hsv.x, .y = hsv.y, .z = hsv.z, .w = rgba.w};

	return hsva;
}

static inline v4
color_rgba_from_hsva(v4 hsva)
{
	v3 hsv  = {.x = hsva.x, .y = hsva.y, .z = hsva.z};
	v3 rgb  = color_rgb_from_hsv(hsv);
	v4 rgba = {.x = rgb.x, .y = rgb.y, .z = rgb.z, .w = hsva.w};
	return rgba;
}

// srgb <-> linear
static inline v3
color_linear_from_srgb(v3 srgb)
{
	v3 result;

	for(ssize idx = 0; idx < (ssize)ARRLEN(srgb.v); idx++) {
		result.v[idx] = (srgb.v[idx] < 0.0404482362771082f ? srgb.v[idx] / 12.92f : pow_f32((srgb.v[idx] + 0.055f) / 1.055f, 2.4f));
	}
	return result;
}

static inline v3
color_srgb_from_linear(v3 linear)
{
	v3 result;
	for(ssize idx = 0; idx < (ssize)ARRLEN(linear.v); idx++) {
		result.v[idx] = (linear.v[idx] < 0.00313066844250063f) ? linear.v[idx] * 12.92f : 1.055f * pow_f32(linear.v[idx], 1.f / 2.4f) - 0.055f;
	}
	return result;
}

static inline v4
color_linear_from_srgba(v4 srgba)
{
	v4 result;
	result.xyz = color_linear_from_srgb(srgba.xyz);
	result.w   = srgba.w;
	return result;
}

static inline v4
color_srgba_from_linear(v4 linear)
{
	v4 result;
	result.xyz = color_srgb_from_linear(linear.xyz);
	result.w   = linear.w;
	return result;
}

// oklab <-> linear
static inline v3
color_oklab_from_linear(v3 linear)
{
	f32 l  = (0.4122214708f * linear.x + 0.5363325363f * linear.y + 0.0514459929f * linear.z);
	f32 m  = (0.2119034982f * linear.x + 0.6806995451f * linear.y + 0.1073969566f * linear.z);
	f32 s  = (0.0883024619f * linear.x + 0.2817188376f * linear.y + 0.6299787005f * linear.z);
	f32 ll = cbrt_f32(l);
	f32 mm = cbrt_f32(m);
	f32 ss = cbrt_f32(s);
	v3 result;
	result.x = 0.2104542553f * ll + 0.7936177850f * mm - 0.0040720468f * ss;
	result.y = 1.9779984951f * ll - 2.4285922050f * mm + 0.4505937099f * ss;
	result.z = 0.0259040371f * ll + 0.7827717662f * mm - 0.8086757660f * ss;
	return result;
}

static inline v3
color_linear_from_oklab(v3 oklab)
{
	f32 ll = oklab.x + 0.3963377774f * oklab.y + 0.2158037573f * oklab.z;
	f32 mm = oklab.x - 0.1055613458f * oklab.y - 0.0638541728f * oklab.z;
	f32 ss = oklab.x - 0.0894841775f * oklab.y - 1.2914855480f * oklab.z;
	f32 l  = ll * ll * ll;
	f32 m  = mm * mm * mm;
	f32 s  = ss * ss * ss;
	v3 result;
	result.x = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
	result.y = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
	result.z = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
	return result;
}

static inline v4
color_oklab_from_lineara(v4 lineara)
{
	v4 result;
	result.xyz = color_oklab_from_linear(lineara.xyz);
	result.w   = lineara.w;
	return result;
}

static inline v4
color_lineara_from_oklab(v4 oklab)
{
	v4 result;
	result.xyz = color_linear_from_oklab(oklab.xyz);
	result.w   = oklab.w;
	return result;
}
