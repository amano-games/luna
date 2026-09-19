#include "pck.h"

#include "base/dbg.h"
#include "base/hash.h"
#include "base/types.h"
#include "sys/sys-io.h"

static u16
pck_read_u16(sys_file fh)
{
	u8 b[sizeof(u16)] = {0};
	if(sys_file_r(fh, b, sizeof(u16)) != (ssize)sizeof(u16)) {
		return 0;
	}
	return (b[1] << 8) | b[0];
}

static u32
pck_read_u32(sys_file fh)
{
	u8 b[sizeof(u32)] = {0};
	if(sys_file_r(fh, b, sizeof(u32)) != (ssize)sizeof(u32)) {
		return 0;
	}
	return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

static u64
pck_read_u64(sys_file fh)
{
	u8 b[sizeof(u64)] = {0};
	if(sys_file_r(fh, b, sizeof(u64)) != (ssize)sizeof(u64)) {
		return 0;
	}
	return ((u64)b[7] << 56) | ((u64)b[6] << 48) |
		((u64)b[5] << 40) | ((u64)b[4] << 32) |
		((u64)b[3] << 24) | ((u64)b[2] << 16) |
		((u64)b[1] << 8) | ((u64)b[0]);
}

i32
pck_open(str8 path, struct pck_desc *pack)
{
	i32 res     = 0;
	sys_file fh = sys_file_open_r(path);

	dbg_check(sys_file_is_valid(fh), "pck", "failed to open %.*s", str8_spread(path));

	sys_file_seek_end(fh, 0);
	i32 size = sys_file_tell(fh);

	dbg_check(
		(size > PCK_HEADER_SIZE && sys_file_seek_set(fh, size - PCK_HEADER_SIZE) == 0),
		"pck",
		"invalid file: %.*s",
		str8_spread(path));

	ssize index_len    = pck_read_u32(fh);
	ssize archive_size = pck_read_u32(fh);
	ssize magic        = pck_read_u32(fh);

	// Check magic, make sure index_len is possible with the file size
	dbg_check(
		(magic == PCK_MAGIC) && index_len * PCK_INDEX_SIZE <= (ssize)(size - PCK_HEADER_SIZE),
		"pck",
		"invalid file: %.*s",
		str8_spread(path));

	// Find a good size for the hashmap: power of 2, at least 1.5x num entries
	ssize hashmap_len     = 1;
	ssize min_hashmap_len = index_len * 1.5;
	while(hashmap_len < min_hashmap_len) {
		hashmap_len <<= 1;
	}

	pack->fh           = fh;
	pack->ht           = NULL;
	pack->files_offset = size - archive_size;
	pack->index_len    = index_len;
	pack->index_offset = size - pack->index_len * PCK_INDEX_SIZE - PCK_HEADER_SIZE;
	pack->ht_len       = hashmap_len;
	pack->ht_size      = pack->ht_len * sizeof(struct pck_file);
	res                = size;

error:;
	if(res == 0 && sys_file_is_valid(fh)) { sys_file_close(fh); }
	return res;
}

i32
pck_read_index(struct pck_desc *pack, void *buffer)
{
	pack->ht = buffer;
	i32 mask = pack->ht_len - 1;

	mclr(pack->ht, pack->ht_size);
	sys_file_seek_set(pack->fh, pack->index_offset);

	for(ssize i = 0; i < pack->index_len; i++) {
		u64 hash = pck_read_u64(pack->fh);

		i32 idx = hash & mask;
		while(pack->ht[idx].size > 0) {
			idx = (idx + 1) & mask;
		}
		pack->ht[idx].hash      = hash;
		pack->ht[idx].offset    = pck_read_u32(pack->fh);
		pack->ht[idx].size      = pck_read_u32(pack->fh);
		pack->ht[idx].base_size = pck_read_u32(pack->fh);
		pack->ht[idx].path_len  = pck_read_u16(pack->fh);
		pack->ht[idx].flags     = pck_read_u16(pack->fh);
	}
	return pack->index_len;
}

void
pck_close(struct pck_desc *pack)
{
	sys_file_close(pack->fh);
}

struct pck_file *
pck_find_hash(struct pck_desc *pack, u64 hash)
{
	struct pck_file *res = NULL;
	i32 mask;
	i32 idx;

	if(pack->ht == NULL) {
		goto done;
	}

	mask = pack->ht_len - 1;
	idx  = hash & mask;
	while(pack->ht[idx].size > 0) {
		if(pack->ht[idx].hash == hash) {
			res = &pack->ht[idx];
			goto done;
		}
		idx = (idx + 1) & mask;
	}

done:
	return res;
}

struct pck_file *
pck_find(struct pck_desc *pack, str8 path)
{
	return pck_find_hash(pack, hash_fnv1a_str8(path));
}

i32
pck_read_path(struct pck_desc *pack, struct pck_file *file, char *dest)
{
	sys_file_seek_set(pack->fh, pack->files_offset + file->offset);
	return (i32)sys_file_r(pack->fh, dest, file->path_len);
}

static ssize
pck_payload_off(struct pck_desc *pack, struct pck_file *file)
{
	return pack->files_offset + file->offset + file->path_len;
}

i32
pck_seek(struct pck_desc *pack, struct pck_file *file, ssize start)
{
	return sys_file_seek_set(pack->fh, pck_payload_off(pack, file) + start);
}

i32
pck_read_cur(struct pck_desc *pack, u8 *dest, ssize len)
{
	return (i32)sys_file_r(pack->fh, dest, len);
}

i32
pck_read(struct pck_desc *pack, struct pck_file *file, u8 *dest)
{
	pck_seek(pack, file, 0);
	return pck_read_cur(pack, dest, file->size);
}

i32
pck_read_ex(struct pck_desc *pack, struct pck_file *file, u8 *dest, ssize start, ssize len)
{
	pck_seek(pack, file, start);
	return pck_read_cur(pack, dest, len);
}
