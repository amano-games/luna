#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

size_t
write_file(char *file_name, uint32_t memory_size, void *memory)
{
	int32_t file_handle = open(
		file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if(file_handle == -1) { return false; };

	uint32_t bytes_to_write = memory_size;

	uint8_t *next_byte_location = (uint8_t *)memory;

	while(bytes_to_write) {
		ssize_t bytes_written =
			write(file_handle, next_byte_location, bytes_to_write);

		if(bytes_written == -1) {
			close(file_handle);
			return false;
		}

		bytes_to_write -= bytes_written;
		next_byte_location += bytes_written;
	};

	close(file_handle);

	return true;
}
