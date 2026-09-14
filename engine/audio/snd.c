#include "snd.h"
#include "base/dbg.h"
#include "base/utils.h"
#include "sys/sys-io.h"

struct snd
snd_load_from_mem(struct alloc alloc, void *data, ssize size)
{
	struct snd res = {0};
	u8 *src        = data;

	dbg_check(data, "snd", "null snd data");
	dbg_check(size >= (ssize)sizeof(u32), "snd", "snd blob too small");

	struct snd_header snd_header = {0};
	mcpy(&snd_header, src, sizeof(u32));
	u32 bytes = (snd_header.sample_count + 1) >> 1;
	dbg_check(size >= (ssize)sizeof(u32) + (ssize)bytes, "snd", "snd blob truncated");

	u8 *buf = alloc_size_aligned(alloc, bytes, alignof(u8), false);
	dbg_check(buf, "snd", "failed to allocate memory for snd");
	mcpy(buf, src + sizeof(u32), bytes);

	res.buf = buf;
	res.len = snd_header.sample_count;

error:;
	return res;
}

struct snd
snd_load(const str8 path, struct alloc alloc)
{
	struct snd res = {0};
	sys_file f     = sys_file_open_r(path);

	dbg_check_warn(sys_file_is_valid(f), "snd", "failed to open file %s", path.str);

	struct snd_header snd_header = {0};
	sys_file_r(f, &snd_header, sizeof(u32));
	u32 bytes = (snd_header.sample_count + 1) >> 1;

	u8 *buf = alloc_size_aligned(alloc, bytes, alignof(u8), false);
	dbg_check_warn(buf, "snd", "failed to allocate memory for snd %s", path.str);

	sys_file_r(f, buf, bytes);

	res.buf = buf;
	res.len = snd_header.sample_count;

error:;
	if(sys_file_is_valid(f)) {
		sys_file_close(f);
	}
	return res;
}
