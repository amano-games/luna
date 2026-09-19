#include "tex-atlas.h"

#include "base/dbg.h"
#include "base/types.h"

b32
atlas_to_blob(struct alloc alloc, struct tex_atlas atlas, struct asset_blob *out)
{
	b32 res        = false;
	void *out_data = NULL;

	dbg_check(out, "atlas", "null blob");
	dbg_check(atlas.cell_w > 0, "atlas", "bad cell_w");
	dbg_check(atlas.cell_h > 0, "atlas", "bad cell_h");

	out_data = mem_alloc_size(alloc, sizeof(atlas));
	dbg_check_mem(out_data, "atlas");
	mcpy(out_data, &atlas, sizeof(atlas));

	out->data = out_data;
	out->size = (ssize)sizeof(atlas);
	res       = true;

error:;
	return res;
}

struct tex_atlas
atlas_from_mem(void *data, ssize size)
{
	struct tex_atlas res = {0};

	dbg_check(data, "atlas", "null atlas data");
	dbg_check(size >= (ssize)sizeof(res), "atlas", "atlas blob too small");

	mcpy(&res, data, sizeof(res));
	dbg_check(res.cell_w > 0 && res.cell_h > 0, "atlas", "invalid cell size");

error:;
	return res;
}
