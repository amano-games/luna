// Sys layer unity includes
// Shared core once, then exactly one platform.

#include "sys/sys-io.c"
#include "sys/sys-sprintf.c"

#if !SYS_BACKEND_CLI
#include "sys/sys.c"
#include "sys/sys-opts.c"
#endif

#if SYS_BACKEND_CLI
#include "sys/cli/sys-cli.c"
#elif OS_PLAYDATE
#include "sys/playdate/sys-playdate.c"
#elif OS_LINUX
#include "sys/linux/sys-linux.c"
#elif OS_MACOS
#include "sys/macos/sys-macos.c"
#elif OS_WINDOWS
#include "sys/windows/sys-windows.c"
#elif OS_WASM
#include "sys/wasm/sys-wasm.c"
#else
#error No platform selected for sys-inc.c
#endif
