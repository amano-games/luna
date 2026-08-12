// Sys layer unity includes
// Shared core once, then exactly one platform.

#include "sys/sys-io.c"
#include "sys/sys-sprintf.c"
#include "sys/sys-log.c"
#include "sys/sys-mem.c"

#if SYS_GFX_SOKOL
#include "sys/sys.c"
#include "sys/sys-opts.c"
#endif

#if OS_LINUX
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
