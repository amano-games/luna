#pragma once

/*

Based on QOP (https://phoboslab.org/log/2024/09/qop).

struct {
	// Data of all files, back to back, ordered by extension then path
	uint8_t file_data[];

	// Every path, null terminated, in the same order as the index
	char path_table[path_table_size];

	// The index, in the same order as file_data
	struct {
		uint64_t hash;      // fnv1a of the path
		uint32_t offset;    // into file_data
		uint32_t size;
		uint32_t base_size;
		uint32_t path_off;  // into path_table
		uint16_t path_len;  // including the null terminator
		uint16_t flags;     // pck flags (compression, encryption)
		uint32_t tags;      // opaque to the pck, set by the asset pipeline
	} pck_file[];

	// The size of the path table in bytes
	uint32_t path_table_size;

	// The number of files in the index
	uint32_t index_len;

	// The size of the whole archive, including the header
	uint32_t archive_size;

	// Magic bytes "lpck"
	uint32_t magic;
} pck;

*/

#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "sys/sys-io.h"

#define PCK_EXT         "pck"
#define PCK_HEADER_SIZE 16
#define PCK_INDEX_SIZE  32
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
	ssize path_off;
	u16 path_len;
	u16 flags;
	u32 tags;
};

struct pck_desc {
	sys_file fh;
	ssize pos; // current file position, -1 if unknown
	struct pck_file *ht;
	u32 *order;  // ht slots in index (file) order
	char *paths; // path table
	ssize files_offset;
	ssize paths_offset;
	ssize paths_size;
	ssize index_offset;
	ssize index_len;
	ssize ht_len;
	ssize mem_size; // ht + order + path table, see pck_read_index
};

struct pck_desc pck_open(str8 path);
void pck_close(struct pck_desc *pack);

i32 pck_read_index(struct alloc scratch, struct pck_desc *pack, void *buffer);

struct pck_file *pck_file_at(struct pck_desc *pack, ssize i);
str8 pck_path(struct pck_desc *pack, struct pck_file *file);

struct pck_file *pck_find(struct pck_desc *pack, str8 path);
struct pck_file *pck_find_hash(struct pck_desc *pack, u64 hash);
str8 pck_path_copy(struct alloc alloc, struct pck_desc *pack, struct pck_file *file);
i32 pck_read(struct pck_desc *pack, struct pck_file *file, u8 *dest);
i32 pck_read_ex(struct pck_desc *pack, struct pck_file *file, u8 *dest, ssize start, ssize len);
i32 pck_seek(struct pck_desc *pack, struct pck_file *file, ssize start);
i32 pck_read_cur(struct pck_desc *pack, u8 *dest, ssize len);
