#include "snd.h"
#include "sys/sys-io.h"
#include "base/log.h"

struct snd
snd_load(const str8 path, struct alloc alloc)
{
	struct snd res = {0};

	void *f = sys_file_open_r(path);
	if(!f) {
		log_warn("snd", "Can't open file %s\n", path.str);
		return res;
	}

	u32 num_samples = 0;
	sys_file_r(f, &num_samples, sizeof(u32));
	u32 bytes = (num_samples + 1) >> 1;

	void *buf = alloc.allocf(alloc.ctx, bytes);
	if(!buf) {
		log_error("snd", "Can't allocate memory for snd: %s\n", path.str);
		sys_file_close(f);
		return res;
	}

	sys_file_r(f, buf, bytes);
	sys_file_close(f);

	res.buf = (u8 *)buf;
	res.len = num_samples;
	return res;
}
