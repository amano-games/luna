#include "snd.h"
#include "base/dbg.h"
#include "sys/sys-io.h"

struct snd
snd_load(const str8 path, struct alloc alloc)
{
	struct snd res = {0};
	void *f        = sys_file_open_r(path);

	dbg_check_warn(f, "snd", "failed to open file %s", path.str);

	struct snd_header snd_header = {0};
	sys_file_r(f, &snd_header, sizeof(u32));
	u32 bytes = (snd_header.sample_count + 1) >> 1;

	void *buf = alloc.allocf(alloc.ctx, bytes, 4);
	dbg_check_warn(buf, "snd", "failed to allocate memory for snd %s", path.str);

	sys_file_r(f, buf, bytes);

	res.buf = (u8 *)buf;
	res.len = snd_header.sample_count;

error:;
	if(f) {
		sys_file_close(f);
	}
	return res;
}
