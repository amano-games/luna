#include "asset.h"
#include "base/dbg.h"
#include "sys/sys-io.h"

b32
asset_blob_w(struct asset_blob blob, str8 out_path)
{
	b32 res      = false;
	sys_file file = sys_file_open_w(out_path);
	dbg_check(sys_file_is_valid(file), "asset", "failed to open file to write: %s", out_path.str);
	dbg_check(sys_file_w(file, blob.data, blob.size) == (ssize)blob.size, "asset", "Error writing asset to file: %s", out_path.str);

	res = true;
error:;
	if(sys_file_is_valid(file)) { sys_file_close(file); }
	return res;
}
