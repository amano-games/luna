#include "asset.h"
#include "base/dbg.h"
#include "base/log.h"
#include "base/mem.h"
#include "lib/tex/tex.h"
#include "sys/sys-io.h"
#include "sys/sys-lz4hc.h"
#include "sys/sys.h"

b32
asset_blob_w(struct asset_blob blob, str8 out_path)
{
	b32 res       = false;
	sys_file file = sys_file_open_w(out_path);
	dbg_check(sys_file_is_valid(file), "asset", "failed to open file to write: %s", out_path.str);
	dbg_check(sys_file_w(file, blob.data, blob.size) == (ssize)blob.size, "asset", "Error writing asset to file: %s", out_path.str);

	res = true;
error:;
	if(sys_file_is_valid(file)) { sys_file_close(file); }
	return res;
}

static ssize
tex_px_lz4hc(struct alloc scratch, struct tex t, void **out, u32 *flags)
{
	ssize res = 0;
	ssize raw = 0;
	u8 *lz4   = NULL;
	int bound = 0;
	int n     = 0;

	dbg_check(out && flags, "tex", "lz4hc null out");
	dbg_check(t.px, "tex", "lz4hc null px");

	raw    = (ssize)sizeof(u32) * t.wword * t.h;
	*out   = t.px;
	*flags = TEX_FLAG_NONE;
	res    = raw;

	if(raw <= 0 || raw > SYS_LZ4_MAX_INPUT) {
		goto cleanup;
	}

	bound = sys_lz4_compress_bound(raw);
	if(bound <= 0) {
		goto cleanup;
	}

	lz4 = mem_alloc_size(scratch, (usize)bound);
	dbg_check(lz4, "tex", "lz4hc alloc failed");

	n = sys_lz4_compress_hc(t.px, lz4, raw, bound);

	if(n <= 0 || n >= (int)raw) {
		goto cleanup;
	}

	*out   = lz4;
	*flags = TEX_FLAG_LZ4;
	res    = (ssize)n;

cleanup:
error:;
	return res;
}

b32
tex_to_blob(struct alloc scratch, struct alloc alloc, struct tex t, struct asset_blob *out, enum tex_px_enc enc)
{
	b32 res        = false;
	void *px       = NULL;
	u32 flags      = TEX_FLAG_NONE;
	ssize packed   = 0;
	ssize out_size = 0;
	void *out_data = NULL;

	dbg_check(out, "tex", "null blob");
	dbg_check(t.px, "tex", "null px");

	if(enc == TEX_PX_LZ4HC) {
		packed = tex_px_lz4hc(scratch, t, &px, &flags);
	} else {
		px     = t.px;
		flags  = TEX_FLAG_NONE;
		packed = (ssize)sizeof(u32) * t.wword * t.h;
	}

	out_size = (ssize)sizeof(struct tex_header) + packed;
	out_data = mem_alloc_size(alloc, out_size);
	dbg_check_mem(out_data, "tex");

	struct tex_header header = {
		.fmt   = (u32)t.fmt,
		.w     = (u32)t.w,
		.h     = (u32)t.h,
		.flags = flags,
	};
	mcpy(out_data, &header, sizeof(header));
	mcpy((u8 *)out_data + sizeof(header), px, packed);

	out->data = out_data;
	out->size = out_size;
	res       = true;

error:;
	return res;
}
