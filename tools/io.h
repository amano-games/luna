#pragma once

#include "sys-io.h"
#include "sys-types.h"

size_t
write_full_file(str8 path, uint32_t memory_size, void *memory)
{
	void *file = sys_file_open_w(path);

	if(file == NULL) { return false; };

	uint32_t bytes_to_write = memory_size;

	uint8_t *next_byte_location = (uint8_t *)memory;

	while(bytes_to_write) {
		i32 bytes_written =
			sys_file_w(file, next_byte_location, bytes_to_write);

		if(bytes_written == -1) {
			sys_file_close(file);
			return false;
		}

		bytes_to_write -= bytes_written;
		next_byte_location += bytes_written;
	};

	sys_file_close(file);

	return true;
}
