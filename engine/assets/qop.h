#pragma once

/*
-- File format description (pseudo code)

struct {
	// Path string and data of all files in this archive
	struct {
		uint8_t path[path_len];
		uint8_t bytes[size];
	} file_data[];

	// The index, with a list of files
	struct {
		uint64_t hash;
		uint32_t offset;
		uint32_t size;
		uint16_t path_len;
		uint16_t flags;
	} qop_file[];

	// The number of files in the index
	uint32_t index_len;

	// The size of the whole archive, including the header
	uint32_t archive_size; 

	// Magic bytes "qopf"
	uint32_t magic;
} qop;


*/

#include "base/types.h"
#define QOP_FLAG_NONE               0
#define QOP_FLAG_COMPRESSED_ZSTD    (1 << 0)
#define QOP_FLAG_COMPRESSED_DEFLATE (1 << 1)
#define QOP_FLAG_ENCRYPTED          (1 << 8)

typedef struct {
	u64 hash;
	u32 offset;
	u32 size;
	u16 path_len;
	u16 flags;
} qop_file;

typedef struct {
	void *fh;
	qop_file *hashmap;
	u32 files_offset;
	u32 index_offset;
	u32 index_len;
	u32 hashmap_len;
	u32 hashmap_size;
} qop_desc;

// Open an archive at path. The supplied qop_desc will be filled with the
// information from the file header. Returns the size of the archvie or 0 on
// failure.
i32 qop_open(str8 path, qop_desc *qop);

// Read the index from an opened archive. The supplied buffer will be filled
// with the index data and must be at least qop->hashmap_size bytes long.
// No ownership is taken of the buffer; if you allocated it with malloc() you
// need to free() it yourself after qop_close();
// Returns the number of files in the archive or 0 on error.
i32 qop_read_index(qop_desc *qop, void *buffer);

// Close the archive.
void qop_close(qop_desc *qop);

// Find a file with the supplied path. Returns NULL if the file is not found.
qop_file *qop_find(qop_desc *qop, str8 path);

// Copy the path of the file into dest. The dest buffer must be at least
// file->path_len bytes long. The path is null terminated.
// Returns the path length (including the null terminater) or 0 on error.
i32 qop_read_path(qop_desc *qop, qop_file *file, char *dest);

// Read the whole file into dest. The dest buffer must be at least file->size
// bytes long.
// Returns the number of bytes read.
i32 qop_read(qop_desc *qop, qop_file *file, u8 *dest);

// Read part of a file into dest. The dest buffer must be at least len bytes
// long.
// Returns the number of bytes read.
i32 qop_read_ex(qop_desc *qop, qop_file *file, u8 *dest, u32 start, u32 len);
