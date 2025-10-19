#if defined(BACKEND_SOKOL)
#if !defined(TARGET_WASM)
#include "whereami.c"
#endif
#endif

#include "sys/sys-io.c"
#include "sys/sys.c"
#if defined BACKEND_SOKOL
#include "sys/sys-sokol.c"
#else
#include "sys/sys-pd.c"
#endif

#include "base/date-time.c"
#include "base/marena.c"
#include "base/mem.c"
#include "base/path.c"
#include "base/str.c"
#include "base/trace.c"
#include "engine/animation/animation-db.c"
#include "engine/animation/animation.c"
#include "engine/animation/animator.c"
#include "engine/assets/asset-db.c"
#include "engine/assets/assets.c"
#include "engine/audio/adpcm.c"
#include "engine/audio/audio.c"
#include "engine/audio/snd.c"
#include "engine/cam/cam-brain.c"
#include "engine/cam/cam.c"
#include "engine/collisions/collisions-ser.c"
#include "engine/collisions/collisions.c"
#include "engine/dbg-drw/dbg-drw-cam.c"
#include "engine/dbg-drw/dbg-drw.c"
#include "engine/gfx/gfx-spr.c"
#include "engine/gfx/gfx-txt.c"
#include "engine/gfx/gfx.c"
#include "engine/input.c"
#include "engine/physics/body-ser.c"
#include "engine/physics/physics.c"
#include "lib/bet/bet-ser.c"
#include "lib/bet/bet.c"
#include "lib/fnt/fnt.c"
#include "lib/pd-utils.c"
#include "lib/pinb/pinb-ser.c"
#include "lib/rndm.c"
#include "lib/ss-grid.c"
#include "lib/tex/tex.c"
