#include "pck.h"

#include "base/dbg.h"
#include "base/hash.h"
#include "base/types.h"
#include "sys/sys-io.h"

static u32
pck_u32_le(const u8 *b)
{
	return ((u32)b[3] << 24) | ((u32)b[2] << 16) | ((u32)b[1] << 8) | (u32)b[0];
}

static u16
pck_u16_le(const u8 *b)
{
	return (u16)(((u16)b[1] << 8) | (u16)b[0]);
}

static u64
pck_u64_le(const u8 *b)
{
	return ((u64)pck_u32_le(b + 4) << 32) | (u64)pck_u32_le(b);
}

struct pck_desc
pck_open(str8 path)
{
	struct pck_desc res = {.fh = sys_file_zero(), .pos = -1};
	sys_file fh         = sys_file_open_r(path);
	u8 footer[PCK_HEADER_SIZE];

	dbg_check(sys_file_is_valid(fh), "pck", "failed to open %.*s", str8_spread(path));

	sys_file_seek_end(fh, 0);
	i32 size = sys_file_tell(fh);

	dbg_check(
		(size > PCK_HEADER_SIZE && sys_file_seek_set(fh, size - PCK_HEADER_SIZE) == 0),
		"pck",
		"invalid file: %.*s",
		str8_spread(path));
	dbg_check(sys_file_r(fh, footer, PCK_HEADER_SIZE) == PCK_HEADER_SIZE, "pck", "invalid file: %.*s", str8_spread(path));

	ssize paths_size   = pck_u32_le(footer + 0);
	ssize index_len    = pck_u32_le(footer + 4);
	ssize archive_size = pck_u32_le(footer + 8);
	ssize magic        = pck_u32_le(footer + 12);

	// Check magic, make sure the index and path table fit in the file
	dbg_check(
		(magic == PCK_MAGIC) &&
			index_len * PCK_INDEX_SIZE + paths_size <= (ssize)(size - PCK_HEADER_SIZE) &&
			archive_size <= size,
		"pck",
		"invalid file: %.*s",
		str8_spread(path));

	// Find a good size for the hashmap: power of 2, at least 1.5x num entries
	ssize hashmap_len     = 1;
	ssize min_hashmap_len = index_len * 1.5;
	while(hashmap_len < min_hashmap_len) {
		hashmap_len <<= 1;
	}

	res.fh           = fh;
	res.files_offset = size - archive_size;
	res.index_len    = index_len;
	res.index_offset = size - index_len * PCK_INDEX_SIZE - PCK_HEADER_SIZE;
	res.paths_size   = paths_size;
	res.paths_offset = res.index_offset - paths_size;
	res.ht_len       = hashmap_len;
	res.mem_size     = (ssize)(hashmap_len * sizeof(struct pck_file)) +
		(ssize)(index_len * sizeof(u32)) +
		paths_size;
	return res;

error:;
	if(sys_file_is_valid(fh)) { sys_file_close(fh); }
	return res;
}

i32
pck_read_index(struct alloc scratch, struct pck_desc *pack, void *buffer)
{
	i32 res      = 0;
	i32 mask     = pack->ht_len - 1;
	u8 *mem      = (u8 *)buffer;
	ssize raw_sz = pack->index_len * PCK_INDEX_SIZE;
	u8 *raw      = NULL;

	pack->ht    = (struct pck_file *)mem;
	pack->order = (u32 *)(mem + pack->ht_len * sizeof(struct pck_file));
	pack->paths = (char *)(pack->order + pack->index_len);
	mclr(pack->ht, pack->ht_len * sizeof(struct pck_file));

	if(pack->paths_size > 0) {
		dbg_check(sys_file_seek_set(pack->fh, pack->paths_offset) == 0, "pck", "path table seek failed");
		dbg_check(sys_file_r(pack->fh, pack->paths, (u32)pack->paths_size) == pack->paths_size, "pck", "path table read failed");
	}

	raw = mem_alloc_size(scratch, (usize)MAX(raw_sz, 1));
	dbg_check(raw, "pck", "index scratch alloc failed");
	dbg_check(sys_file_seek_set(pack->fh, pack->index_offset) == 0, "pck", "index seek failed");
	dbg_check(sys_file_r(pack->fh, raw, (u32)raw_sz) == raw_sz, "pck", "index read failed");

	pack->pos = pack->index_offset + raw_sz;

	for(ssize i = 0; i < pack->index_len; i++) {
		const u8 *e = raw + i * PCK_INDEX_SIZE;
		u64 hash    = pck_u64_le(e);
		i32 idx     = hash & mask;

		while(pack->ht[idx].path_len > 0) {
			idx = (idx + 1) & mask;
		}

		pack->ht[idx] = (struct pck_file){
			.hash      = hash,
			.offset    = pck_u32_le(e + 8),
			.size      = pck_u32_le(e + 12),
			.base_size = pck_u32_le(e + 16),
			.path_off  = pck_u32_le(e + 20),
			.path_len  = pck_u16_le(e + 24),
			.flags     = pck_u16_le(e + 26),
			.tags      = pck_u32_le(e + 28),
		};

		dbg_check(pack->ht[idx].path_len > 0, "pck", "index entry %d has no path", (int)i);
		dbg_check(
			pack->ht[idx].path_off + pack->ht[idx].path_len <= pack->paths_size,
			"pck",
			"index entry %d path out of range",
			(int)i);
		pack->order[i] = (u32)idx;
	}
	res = pack->index_len;

error:;
	return res;
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
	while(pack->ht[idx].path_len > 0) {
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

struct pck_file *
pck_file_at(struct pck_desc *pack, ssize i)
{
	dbg_assert(pack->order && i >= 0 && i < pack->index_len);
	return &pack->ht[pack->order[i]];
}

str8
pck_path(struct pck_desc *pack, struct pck_file *file)
{
	return (str8){
		.str  = (u8 *)pack->paths + file->path_off,
		.size = (usize)file->path_len - 1,
	};
}

str8
pck_path_copy(struct alloc alloc, struct pck_desc *pack, struct pck_file *file)
{
	if(pack->paths == NULL || file->path_len == 0) { return (str8){0}; }
	return str8_cpy_push(alloc, pck_path(pack, file));
}

static ssize
pck_payload_off(struct pck_desc *pack, struct pck_file *file)
{
	return pack->files_offset + file->offset;
}

i32
pck_seek(struct pck_desc *pack, struct pck_file *file, ssize start)
{
	ssize target = pck_payload_off(pack, file) + start;
	i32 res      = sys_file_seek_set(pack->fh, target);
	pack->pos    = res == 0 ? target : -1;
	return res;
}

i32
pck_read_cur(struct pck_desc *pack, u8 *dest, ssize len)
{
	i32 res = (i32)sys_file_r(pack->fh, dest, len);
	if(res > 0 && pack->pos >= 0) {
		pack->pos += res;
	} else if(res < 0) {
		pack->pos = -1;
	}
	return res;
}

i32
pck_read(struct pck_desc *pack, struct pck_file *file, u8 *dest)
{
	return pck_read_ex(pack, file, dest, 0, file->size);
}

i32
pck_read_ex(struct pck_desc *pack, struct pck_file *file, u8 *dest, ssize start, ssize len)
{
	// Skip seeks when possible
	if(pack->pos != pck_payload_off(pack, file) + start) {
		pck_seek(pack, file, start);
	}
	return pck_read_cur(pack, dest, len);
}
