#pragma once

#include <ctype.h>
#include <stdint.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x0100)

// Function to convert a 32-bit unsigned integer to big-endian
uint32_t
to_big_endian(uint32_t value)
{
	return ((value & 0x000000FF) << 24) |
		((value & 0x0000FF00) << 8) |
		((value & 0x00FF0000) >> 8) |
		((value & 0xFF000000) >> 24);
}

uint32_t
safe_truncate_u_int64(uint64_t value)
{
	assert(value <= 0xFFFFFFFF);
	uint32_t result = (uint32_t)value;
	return (result);
}

void
get_basename(const char *path, char *output)
{
	// Find the last occurrence of the path separator
	const char *last_slash = strrchr(path, '/');

	// #ifdef _WIN32
	// 	// On Windows, check for backslashes as well
	// 	const char *last_backslash = strrchr(path, '\\');
	// 	if(last_backslash && (!last_slash || last_backslash > last_slash)) {
	// 		last_slash = last_backslash;
	// 	}
	// #endif

	// Start after the last path separator, or at the beginning if none is found
	const char *filename = (last_slash) ? last_slash + 1 : path;

	// Find the last occurrence of the dot (.)
	const char *last_dot = strrchr(filename, '.');

	// Copy the filename without the extension
	if(last_dot) {
		strncpy(output, filename, last_dot - filename);
		output[last_dot - filename] = '\0'; // Null-terminate the string
	} else {
		strcpy(output, filename); // No dot found, copy the whole filename
	}
}

void
change_extension(const char *in, char *out, const char *new_extension)
{
	const char *last_dot = strrchr(in, '.'); // Find the last occurrence of '.'
	if(last_dot != NULL) {
		size_t original_length = last_dot - in; // Length of the filename excluding the extension
		if(original_length + 1 + strlen(new_extension) <= FILENAME_MAX) {
			memcpy(out, in, original_length); // Copy the filename part
			out[original_length] = '\0';      // Null terminate the copied part
			strcat(out, ".");                 // Add a dot
			strcat(out, new_extension);       // Add the new extension
		}
	}
}

static void
little_endian_to_native(void *data, char *format)
{
	unsigned char *cp = (unsigned char *)data;
	int32_t temp;

	while(*format) {
		switch(*format) {
		case 'L':
			temp           = cp[0] + ((int32_t)cp[1] << 8) + ((int32_t)cp[2] << 16) + ((int32_t)cp[3] << 24);
			*(int32_t *)cp = temp;
			cp += 4;
			break;

		case 'S':
			temp         = cp[0] + (cp[1] << 8);
			*(short *)cp = (short)temp;
			cp += 2;
			break;

		default:
			if(isdigit((unsigned char)*format))
				cp += *format - '0';

			break;
		}

		format++;
	}
}
