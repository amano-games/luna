// Sys layer unity includes
// Shared core once, then exactly one platform.

#include "sys/sys-io.c"
#include "sys/sys-sprintf.c"
#include "sys/sys-mem.c"
#include "sys/sys-lz4.c"

#if SYS_GFX
#include "sys/sys.c"
#include "sys/sys-opts.c"
#endif

#if OS_PLAYDATE
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

#include "sys/sys-log.c"
#include "sys/sys-img.c"

#if SYS_GFX

#if SYS_STEAM_ENABLED
#include "sys/steam/sys-steam.c"
#else
#include "sys/steam/sys-steam-stub.c"
#endif

#if SYS_SCORES_BACKEND == SYS_SCORES_MOCK
#include "sys/sys-scoreboards-mock.c"
#elif SYS_SCORES_BACKEND == SYS_SCORES_PD
#include "sys/playdate/sys-playdate-scoreboards.c"
#elif SYS_SCORES_BACKEND == SYS_SCORES_STEAM
#include "sys/steam/sys-steam-scoreboards.c"
#elif SYS_SCORES_BACKEND == SYS_SCORES_NONE
#include "sys/sys-scoreboards-none.c"
#endif

#endif
