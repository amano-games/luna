#include "base/base-inc.h"

#include "base/cmd-line.h"
#include "base/log.h"
#include "base/marena.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys.h"
#include "whereami.c"

#include "sys/sys-inc.h"
#include "sys/sys-inc.c"

#include "lib/tex/tex.c"

#include "base/marena.c"
#include "base/str.c"
#include "base/cmd-line.c"
#include "base/path.c"

#define LOG_ID           "asset-pack"
#define DEFAULT_OUT_FILE "assets.qop"

int
main(int argc, char *argv[])
{
	int res                = EXIT_FAILURE;
	struct alloc alloc_sys = sys_allocator();

	usize mem_size = MMEGABYTE(1);
	void *mem      = mem_alloc_size(alloc_sys, mem_size);
	dbg_check_warn(mem, LOG_ID, "Failed to get scratch memory");
	struct marena scratch_arena = {0};
	marena_init(&scratch_arena, mem, mem_size);
	struct alloc scratch = marena_allocator(&scratch_arena);

	struct cmd_line cmd = cmd_line_from_argcv(scratch, argc, argv);

	if(cmd.inputs.node_count < 1) {
		sys_printf(
			"Usage: %.*s <input-folder|input-qop> [output-folder|output-qop]\n"
			"  output defaults to %s\n",
			str8_spread(cmd.exe_name),
			DEFAULT_OUT_FILE);
		res = EXIT_FAILURE;
		goto error;
	}

	str8 in_path  = cmd.inputs.first->str;
	str8 out_path = str8_lit(DEFAULT_OUT_FILE);

	if(cmd.inputs.node_count >= 2) {
		out_path = cmd.inputs.first->next->str;
	}

	log_info(LOG_ID, "Packing assets from %s -> %s", in_path.str, out_path.str);

	res = EXIT_SUCCESS;

error:;
	if(mem) {
		sys_free(mem);
	}

	return res;
}
