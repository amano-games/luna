#pragma once

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
		((hex & 0xff000000) >> 24) / 255.f,
		((hex & 0x00ff0000) >> 16) / 255.f,
		((hex & 0x0000ff00) >> 8) / 255.f,
		((hex & 0x000000ff) >> 0) / 255.f,
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
	v4 rgba = {byte_vals[0] / 255.f, byte_vals[1] / 255.f, byte_vals[2] / 255.f, byte_vals[3] / 255.f};
	return rgba;
}
