#pragma once

/*

Based on QOP (https://phoboslab.org/log/2024/09/qop).

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
		uint32_t base_size;
		uint16_t path_len;
		uint16_t flags;
	} pck_file[];

	// The number of files in the index
	uint32_t index_len;

	// The size of the whole archive, including the header
	uint32_t archive_size;

	// Magic bytes "lpck"
	uint32_t magic;
} pck;


*/

#include "base/types.h"
#include "sys/sys-io.h"

#define PCK_EXT         "pck"
#define PCK_HEADER_SIZE 12
#define PCK_INDEX_SIZE  24
#define PCK_MAGIC \
	(((u32)'l') << 0 | ((u32)'p') << 8 | \
		((u32)'c') << 16 | ((u32)'k') << 24)

enum pck_flag {
	PCK_FLAG_NONE               = 0,
	PCK_FLAG_COMPRESSED_ZSTD    = 1 << 0,
	PCK_FLAG_COMPRESSED_DEFLATE = 1 << 1,
	PCK_FLAG_COMPRESSED_LZ4     = 1 << 2,
	PCK_FLAG_ENCRYPTED          = 1 << 8,
};

struct pck_file {
	u64 hash;
	ssize offset;
	ssize size;
	ssize base_size;
	u16 path_len;
	u16 flags;
};

struct pck_desc {
	sys_file fh;
	struct pck_file *ht;
	ssize files_offset;
	ssize index_offset;
	ssize index_len;
	ssize ht_len;
	ssize ht_size;
};

// Open an archive at path. The supplied pck_desc will be filled with the
// information from the file header. Returns the size of the archvie or 0 on
// failure.
i32 pck_open(str8 path, struct pck_desc *pack);

// Read the index from an opened archive. The supplied buffer will be filled
// with the index data and must be at least pack->hashmap_size bytes long.
// No ownership is taken of the buffer; if you allocated it with malloc() you
// need to free() it yourself after pck_close();
// Returns the number of files in the archive or 0 on error.
i32 pck_read_index(struct pck_desc *pack, void *buffer);

// Close the archive.
void pck_close(struct pck_desc *pack);

// Find a file with the supplied path. Returns NULL if the file is not found.
struct pck_file *pck_find(struct pck_desc *pack, str8 path);

// Find a file by path hash. Returns NULL if the file is not found.
struct pck_file *pck_find_hash(struct pck_desc *pack, u64 hash);

// Copy the path of the file into dest. The dest buffer must be at least
// file->path_len bytes long. The path is null terminated.
// Returns the path length (including the null terminater) or 0 on error.
i32 pck_read_path(struct pck_desc *pack, struct pck_file *file, char *dest);

// Read the whole file into dest. The dest buffer must be at least file->size
// bytes long.
// Returns the number of bytes read.
i32 pck_read(struct pck_desc *pack, struct pck_file *file, u8 *dest);

// Read part of a file into dest. The dest buffer must be at least len bytes
// long.
// Returns the number of bytes read.
i32 pck_read_ex(struct pck_desc *pack, struct pck_file *file, u8 *dest, ssize start, ssize len);

// SEEK_SET to file payload + start (0 = first data byte). Returns 0 on success.
i32 pck_seek(struct pck_desc *pack, struct pck_file *file, ssize start);

// Read from the current handle position. No seek.
i32 pck_read_cur(struct pck_desc *pack, u8 *dest, ssize len);
