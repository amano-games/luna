#include "qop.h"

#include "base/ht.h"
#include "base/types.h"
#include "sys/sys-io.h"

#define QOP_MAGIC \
	(((u32)'q') << 0 | ((u32)'o') << 8 | \
		((u32)'p') << 16 | ((u32)'f') << 24)
#define QOP_HEADER_SIZE 12
#define QOP_INDEX_SIZE  20

static u16
qop_read_u16(void *fh)
{
	u8 b[sizeof(u16)] = {0};
	if(sys_file_r(fh, b, sizeof(u16)) != 1) {
		return 0;
	}
	return (b[1] << 8) | b[0];
}

static u32
qop_read_u32(void *fh)
{
	u8 b[sizeof(u32)] = {0};
	if(sys_file_r(fh, b, sizeof(u32)) != 1) {
		return 0;
	}
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

static u64
qop_read_u64(void *fh)
{
	u8 b[sizeof(u64)] = {0};
	if(sys_file_r(fh, b, sizeof(u64)) != 1) {
		return 0;
	}
	return ((u64)b[7] << 56) | ((u64)b[6] << 48) |
		((u64)b[5] << 40) | ((u64)b[4] << 32) |
		((u64)b[3] << 24) | ((u64)b[2] << 16) |
		((u64)b[1] << 8) | ((u64)b[0]);
}

i32
qop_open(str8 path, qop_desc *qop)
{
	void *fh = sys_file_open_r(path);
	if(!fh) {
		return 0;
	}

	sys_file_seek_end(fh, 0);
	i32 size = sys_file_tell(fh);
	if(size <= QOP_HEADER_SIZE || sys_file_seek_set(fh, size - QOP_HEADER_SIZE) != 0) {
		sys_file_close(fh);
		return 0;
	}

	qop->fh          = fh;
	qop->hashmap     = NULL;
	u32 index_len    = qop_read_u32(fh);
	u32 archive_size = qop_read_u32(fh);
	u32 magic        = qop_read_u32(fh);

	// Check magic, make sure index_len is possible with the file size
	if(
		magic != QOP_MAGIC ||
		index_len * QOP_INDEX_SIZE > (u32)(size - QOP_HEADER_SIZE)) {
		sys_file_close(fh);
		return 0;
	}

	// Find a good size for the hashmap: power of 2, at least 1.5x num entries
	u32 hashmap_len     = 1;
	u32 min_hashmap_len = index_len * 1.5;
	while(hashmap_len < min_hashmap_len) {
		hashmap_len <<= 1;
	}

	qop->files_offset = size - archive_size;
	qop->index_len    = index_len;
	qop->index_offset = size - qop->index_len * QOP_INDEX_SIZE - QOP_HEADER_SIZE;
	qop->hashmap_len  = hashmap_len;
	qop->hashmap_size = qop->hashmap_len * sizeof(qop_file);
	return size;
}

i32
qop_read_index(qop_desc *qop, void *buffer)
{
	qop->hashmap = buffer;
	i32 mask     = qop->hashmap_len - 1;

	mclr(qop->hashmap, qop->hashmap_size);
	sys_file_seek_set(qop->fh, qop->index_offset);

	for(u32 i = 0; i < qop->index_len; i++) {
		u64 hash = qop_read_u64(qop->fh);

		i32 idx = hash & mask;
		while(qop->hashmap[idx].size > 0) {
			idx = (idx + 1) & mask;
		}
		qop->hashmap[idx].hash     = hash;
		qop->hashmap[idx].offset   = qop_read_u32(qop->fh);
		qop->hashmap[idx].size     = qop_read_u32(qop->fh);
		qop->hashmap[idx].path_len = qop_read_u16(qop->fh);
		qop->hashmap[idx].flags    = qop_read_u16(qop->fh);
	}
	return qop->index_len;
}

void
qop_close(qop_desc *qop)
{
	sys_file_close(qop->fh);
}

qop_file *
qop_find(qop_desc *qop, str8 path)
{
	if(qop->hashmap == NULL) {
		return NULL;
	}

	i32 mask = qop->hashmap_len - 1;

	u64 hash = hash_murmuroaat_str8(path);
	i32 idx  = hash & mask;
	while(qop->hashmap[idx].size > 0) {
		if(qop->hashmap[idx].hash == hash) {
			return &qop->hashmap[idx];
		}
		idx = (idx + 1) & mask;
	}
	return NULL;
}

i32
qop_read_path(qop_desc *qop, qop_file *file, char *dest)
{
	sys_file_seek_set(qop->fh, qop->files_offset + file->offset);
	return sys_file_r(qop->fh, dest, file->path_len);
}

i32
qop_read(qop_desc *qop, qop_file *file, u8 *dest)
{
	sys_file_seek_set(qop->fh, qop->files_offset + file->offset + file->path_len);
	return sys_file_r(qop->fh, dest, file->size);
}

i32
qop_read_ex(qop_desc *qop, qop_file *file, u8 *dest, u32 start, u32 len)
{
	sys_file_seek_set(qop->fh, qop->files_offset + file->offset + file->path_len + start);
	return sys_file_r(qop->fh, dest, len);
}
