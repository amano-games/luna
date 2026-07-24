// Sys layer unity includes
// Shared core once, then exactly one backend.

#include "sys/sys-io.c"
#include "sys/sys-sprintf.c"

#if !SYS_BACKEND_CLI
#include "sys/sys.c"
#include "sys/sys-opts.c"
#endif

#if SYS_BACKEND_SOKOL
#include "sys/sokol/sys_sokol.c"
#elif SYS_BACKEND_PLAYDATE
#include "sys/playdate/sys_playdate.c"
#elif SYS_BACKEND_CLI
#include "sys/cli/sys_cli.c"
#else
#error No SYS_BACKEND_* selected for sys_inc.c
#endif
