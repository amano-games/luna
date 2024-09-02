#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "utils.h"

struct read_file_result {
	uint32_t contents_size;
	void *contents;
};

struct read_file_result
read_file(const char *file_name)
{
	struct read_file_result result = {0};

	int32_t file_handle = open(file_name, O_RDONLY);

	if(file_handle == -1) { return result; }

	struct stat file_status;
	if(fstat(file_handle, &file_status) == -1) {
		close(file_handle);
		return result;
	}

	result.contents_size = safe_truncate_u_int64(file_status.st_size);

	result.contents = malloc(result.contents_size);
	if(!result.contents) {
		result.contents_size = 0;
		close(file_handle);
		return result;
	}

	ssize_t bytes_to_read        = result.contents_size;
	uint8_t *next_bytes_location = (uint8_t *)result.contents;

	while(bytes_to_read) {
		uint32_t bytes_read =
			read(file_handle, next_bytes_location, bytes_to_read);

		if(bytes_to_read == -1) {
			free(result.contents);
			result.contents      = 0;
			result.contents_size = 0;
			close(file_handle);
			return result;
		}
		bytes_to_read -= bytes_read;
		next_bytes_location += bytes_read;
	}

	close(file_handle);
	return result;
}

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
