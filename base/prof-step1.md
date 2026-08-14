# Step 1: `PROF_ZONE_HISTORY`

Spec only. Exclusive-time ring, like iProf `Prof_ZONE_HISTORY`. Do not implement `PROF_FRAME_HISTORY`, totals, overhead timers, `__prof`, binary dump, or Superluminal here. That is [`prof-step2.md`](prof-step2.md).

Scope is [`luna/base/prof.h`](prof.h) only. Do not change `sys.c` call sites; `prof_upd(record_data)` is already correct. Overrides go through existing `CDEFS` / `CDEFS_EXTRA` (same as `SYS_LOG_LEVEL`).

## Current behavior

`prof_block_start` / `prof_block_end_internal` accumulate **this present** into `prof.anchors[]`. `prof_upd` blends those samples into EMA slots, then **zeros** `anchors[1..]`.

`PROF_HISTORY_SIZE` is `1` and unused. The commented `history_index = (history_index + 1) % NUM_FRAME_SLOTS` was never wired. EMA (`PROF_SMOOTH_*`) is not a per-frame log.

Anchor `0` (`PROF_ANCHOR_SYS_NONE`) is the root parent. Real zones start at `1`.

## How the knobs work

iProf: `Prof_ZONE_HISTORY` = cheap self-time graph; `Prof_CALL_HISTORY` = full overlay columns per present; `NUM_FRAME_SLOTS` = length. Luna matches that. Do **not** overload `PROF_HISTORY_SIZE == 1` as “no history.”

| iProf | luna | When |
| --- | --- | --- |
| `Prof_ENABLED` | `#if PROF` | overlay / zone enter-exit |
| `Prof_ZONE_HISTORY` | `#if PROF && PROF_ZONE_HISTORY` | **this step** |
| `Prof_CALL_HISTORY` | `#if PROF && PROF_FRAME_HISTORY` | [step 2](prof-step2.md) |
| `NUM_FRAME_SLOTS` (128) | `PROF_HISTORY_SIZE` | shared |

Must use `#if PROF`, not `#if defined(PROF)`. `-DPROF=0` still **defines** `PROF`; `defined(PROF)` would leave the profiler on.

### Defaults (header)

`BUILD_DEBUG` is already `0` or `1` (`-DBUILD_DEBUG=1` in debug, `=0` in release). If a knob is not passed on the compiler command line, it follows debug:

```c
#ifndef PROF
#define PROF BUILD_DEBUG
#endif
#ifndef PROF_ZONE_HISTORY
#define PROF_ZONE_HISTORY PROF
#endif
#ifndef PROF_FRAME_HISTORY
#define PROF_FRAME_HISTORY PROF // honored in step 2; unused code in this step
#endif
#ifndef PROF_HISTORY_SIZE
#define PROF_HISTORY_SIZE 8     // this step; after wrap works, 128 (iprof)
#endif
```

Same pattern as [`log.h`](log.h) / `SYS_LOG_LEVEL`: the header defaults when the macro is **not** on the command line; a `-D` override wins. `linux_dev_build` already appends `CDEFS_EXTRA` to `CDEFS` (see `CDEFS_DEV` / `-DSYS_LOG_LEVEL=2`). No makefile `ifdef` forwarding.

ZONE and FRAME default to **`PROF`**, not raw `BUILD_DEBUG`. `-DPROF=0` turns history off even if ZONE/FRAME were not passed.

`-DPROF=0` → `PROF` is defined as 0 → `#ifndef` does not fire → `#if PROF` is false. Same for ZONE / FRAME.

History writes belong in **`prof_upd` only**. Start/end never see the ring. ZONE/FRAME without `PROF` do nothing (`#if PROF && …`).

### Opt out / opt in

| Command | Effect |
| --- | --- |
| debug (`BUILD_DEBUG=1`), no extra `-D` | `PROF`, `ZONE`, `FRAME` all **1** |
| `CDEFS_EXTRA="-DPROF=0"` | No profiler (stubs). History flags ignored. |
| `CDEFS_EXTRA="-DPROF_ZONE_HISTORY=0"` | Overlay on, no exclusive ring. |
| `CDEFS_EXTRA="-DPROF_FRAME_HISTORY=0"` | No full-present ring (step 2). ZONE still on unless also 0. |
| `BUILD_DEBUG=0`, no extra `-D` | Release: all three default **0**. |
| `BUILD_DEBUG=0` + `CDEFS_EXTRA="-DPROF=1"` | Release overlay. ZONE/FRAME follow PROF unless set to 0. |

Examples:

```
make linux_dev_build CDEFS_EXTRA="-DPROF=0"
make linux_dev_build CDEFS_EXTRA="-DPROF_ZONE_HISTORY=0 -DPROF_FRAME_HISTORY=0"
make linux_dev_build CDEFS_EXTRA="-DPROF_HISTORY_SIZE=1"
```

Do not `#define PROF` in the header when `-DPROF=0` was already passed (`#ifndef` takes care of that). Do not add `ifdef PROF` in `common.mk`.

### What each combo stores

| Build | Behavior |
| --- | --- |
| `PROF=0` (release default) | Stubs. No BSS. |
| `PROF=1`, both history `=0` | Overlay/EMA as today. No ring. Timers 1 / 1 / 1. |
| `PROF=1` + ZONE (debug default this step) | Exclusive µs ring. **~3 KB** at SIZE 8, **~48 KB** at 128. |
| `PROF=1` + FRAME (step 2, also debug default) | Full present ring. See [step 2](prof-step2.md). FRAME supersedes ZONE storage. |
| `SIZE 1` | One slot; each keep overwrites slot `0`. |

Compile-time `#if PROF && PROF_ZONE_HISTORY`. No runtime skip-copy. One push path for all `N >= 1`.

Default **`PROF_HISTORY_SIZE` is 8** so wrap is a few presents. After CP4, change the default to **128**. `1` remains valid (`-DPROF_HISTORY_SIZE=1`). Do not ship 8.

`PROF_HISTORY_SIZE` must be `>= 1`. Ignored when `PROF` is 0 or both history knobs are 0.

The `!PROF` stub `#define PROF_HISTORY_SIZE 1` is unrelated (dummy `struct prof`).

If `PROF_FRAME_HISTORY` is 1 in this step, ignore it (not implemented). Step 2 honors the default and makes FRAME supersede the exclusive array.

## Goals

1. When `PROF_ZONE_HISTORY` is on, keep **exclusive µs** for the last `PROF_HISTORY_SIZE` presents, per zone. Enough for a later sparkline. Not enough to rebuild the overlay or dump a Superluminal-comparable present.
2. Prove wrap, pause, throwaway, and `PROF_HISTORY_SIZE` with almost no extra probe cost (a few `u32` stores per present, not a 1152 B `memcpy`, not extra `sys_time_us` on start/end).
3. Debug (`BUILD_DEBUG=1`): PROF + ZONE on unless passed `=0`. Release / `PROF=0`: **no profiler**. Overlay-only is `PROF_ZONE_HISTORY=0` (and `PROF_FRAME_HISTORY=0`), not the debug default.

## Non-goals

- Inclusive, hit count, `dt_us`, overhead, totals, `__prof` (step 2)
- Drawing the graph (later)
- Binary dump / Superluminal (needs step 2)
- Changing `sys_time_us()` / overlay draw/skip

---

## Memory

iProf layout: `float zone_history[zones][slots]` — one zone is contiguous for a graph.

Luna (µs, 96 zones):

```c
static u32 PROF_ZONE_EXCL[PROF_ANCHORS_SIZE][PROF_HISTORY_SIZE];
```

| N | RAM |
| --- | --- |
| off | **0** |
| 8 | 96 × 4 × 8 = **3 KB** |
| 128 | **48 KB** |

Do **not** embed this in `struct prof`. File-static BSS, pointer from `prof` if needed. `SYS_MAX_MEM` does not need a change.

---

## Data layout

```c
#ifndef PROF
#define PROF BUILD_DEBUG
#endif
#ifndef PROF_ZONE_HISTORY
#define PROF_ZONE_HISTORY PROF
#endif
#ifndef PROF_HISTORY_SIZE
#define PROF_HISTORY_SIZE 8
#endif

struct prof {
	/* existing fields unchanged ... */

#if PROF && PROF_ZONE_HISTORY
	u32 *zone_excl; // -> PROF_ZONE_EXCL[0][0], or just use the static
	u16 history_idx;
	u16 history_filled;
#endif
};

#if PROF && PROF_ZONE_HISTORY
static u32 PROF_ZONE_EXCL[PROF_ANCHORS_SIZE][PROF_HISTORY_SIZE];
#endif
```

`prof_ini` (only if zone history): `history_idx = 0`, `history_filled = 0`. BSS is already zero; no `mclr` of the array at init.

---

## Push (in `prof_upd`)

Start/end unchanged (still 1 timer each). No extra `sys_time_us` in `prof_upd` beyond today’s `dt_us`.

```c
if(record_data) {
#if PROF && PROF_ZONE_HISTORY
	if(prof->update_idx >= PROF_THROWAWAY_UPDATES_COUNT) {
		prof_zone_history_push(prof);
	}
#endif
	/* existing EMA / present_per_s / throwaway */
	++prof->update_idx;
}

if(prof->anchor_count > 0) {
	mclr(prof->anchors + 1, (prof->anchor_count - 1) * sizeof(prof->anchors[0]));
}
```

Copy **before** `mclr`:

```c
#if PROF && PROF_ZONE_HISTORY
static inline void
prof_zone_history_push(struct prof *prof)
{
	u16 s = prof->history_idx;
	for(ssize i = 1; i < prof->anchor_count; ++i) {
		PROF_ZONE_EXCL[i][s] = prof->anchors[i].us_exclusive;
	}
	prof->history_idx = (u16)((s + 1) % PROF_HISTORY_SIZE);
	if(prof->history_filled < PROF_HISTORY_SIZE) {
		++prof->history_filled;
	}
}
#endif
```

Do not copy inclusive or hits. Do not `memcpy` whole `anchors[]`. Unused zones (not in `anchor_count`) stay 0 from BSS / previous wrap — acceptable for a graph.

Only push when `record_data` is true **and** throwaway is over. Pause (`sys_prof_pause` → `record_data` false) freezes the ring.

`SIZE == 1`: idx stays 0; filled saturates at 1. Same function as 8 / 128.

---

## Ring read order

Logical `0` is the **oldest** stored present.

```c
static inline u32
prof_zone_excl_at(u16 zone, u16 logical_i)
{
	dbg_assert(logical_i < PROFILER.history_filled);
	dbg_assert(zone < PROF_ANCHORS_SIZE);
	u16 oldest = 0;
	if(PROFILER.history_filled == PROF_HISTORY_SIZE) {
		oldest = PROFILER.history_idx;
	}
	u16 s = (u16)((oldest + logical_i) % PROF_HISTORY_SIZE);
	return PROF_ZONE_EXCL[zone][s];
}
```

Before wrap: `history_idx == history_filled`, oldest is `0`. After wrap: `filled == SIZE`, oldest is `history_idx`.

---

## Report

Overlay / CSV still EMA. No `__prof` row.

Optional `titles[2]`: `hist %u/%u` → `filled/SIZE`. Useful for checkpoints without a debugger. Skip if you want the overlay pixel-identical; then use gdb for CP1–CP4.

---

## Accessors (stubs when `!PROF` or `!PROF_ZONE_HISTORY`)

```c
u16  prof_history_filled(void);
u16  prof_history_capacity(void); // 0 if flag off, else PROF_HISTORY_SIZE
u32  prof_zone_excl_at(u16 zone, u16 logical_i); // 0 if flag off
```

`capacity == 1` means a one-slot exclusive ring, not “off.” `capacity == 0` means compiled out.

---

## Invariants

- All zones closed at `prof_upd`: `frame_count == 0`, `parent_idx == 0`.
- Loops start at `i = 1`.
- `mclr` still every present, including `record_data` false.
- EMA / overlay unchanged aside from optional `titles[2]`.
- Start/end/`prof_upd` timer counts unchanged (1 / 1 / 1).

---

## Files to touch (this step)

- [`luna/base/prof.h`](prof.h) — `#if PROF` (not `defined(PROF)`), defaults from `BUILD_DEBUG`, ZONE ring. No makefile changes.

Do not change `sys.c` call sites.

---

## Checkpoints

Linux `linux_dev_build` (`BUILD_DEBUG=1`). Overlay on. No device / Superluminal.

Debug default is **PROF + ZONE on**. Overlay-only is `PROF_ZONE_HISTORY=0`. After CP4, default SIZE → 128.

### CP0 — compiles

- `BUILD_DEBUG=0` (or `PROF=0`): stubs. **No** `PROF_ZONE_EXCL` in `nm`.
- Debug default (`linux_dev_build`): `sizeof(PROF_ZONE_EXCL) == 96 * PROF_HISTORY_SIZE * 4` (3072 at 8).
- `PROF=1 PROF_ZONE_HISTORY=0`: links, **no** `PROF_ZONE_EXCL`.
- `PROF=0 PROF_ZONE_HISTORY=1`: no ring (`PROF` gates history).

**Fail if:** `CDEFS_EXTRA="-DPROF=0"` still compiles the ring (`#if defined(PROF)` bug).

### CP0b — overlay-only

`PROF_ZONE_HISTORY=0` (PROF still 1 from debug). `prof_history_filled() == 0`, `capacity() == 0`. Overlay like today. Nested exclusive still matches parent (no extra timers).

### CP1 — throwaway

Debug default (ZONE on). After 2 presents: `filled == 0` while `update_idx < 3`.

### CP2 — 1:1 growth

`filled == min(keeps, SIZE)`. Before wrap, `history_idx == filled`. At 8, wrap after ~8 keeps.

**Fail if:** `filled` jumps by more than 1, grows past `SIZE`, or grows when not presenting.

### CP3 — copy before clear

Break after the store loop. `PROF_ZONE_EXCL[PROF_ANCHOR_SYS_UPD][s]` nonzero (or matches live exclusive). Step `mclr`: live `anchors[1]` is 0; the slot still has the old exclusive.

**Fail if:** all zeros (copied after `mclr`).

### CP4 — wrap

Default 8: `filled` stays 8, `history_idx` cycles `0..7`, logical `0` is the overwritten oldest.

Then default **128**: wrap after `128 / fps`. Optional `SIZE=1`: second keep overwrites slot 0.

### CP5 — pause

`sys_prof_pause()`: `filled` stops. Resume: increases again. EMA may freeze; ring must follow `record_data`.

### CP6 — zone math / overlay unchanged

`PROF_SMOOTH_INSTANT`: overlay matches pre-step (no extra start/end timers). Last exclusive in the ring equals live exclusive just before `mclr`.

### CP10 — `PROF_PRINT`

CSV like today (EMA). Optional `hist filled/SIZE` on a title line. No `__prof`.

### CP11 — `PROF` off

`PROF=0` or `BUILD_DEBUG=0`. Overlay/print macros stay no-ops. No `PROF_ZONE_EXCL`.

---

## Suggested implementation order

1. Header defaults (`PROF` ← `BUILD_DEBUG`, `ZONE` ← `PROF`). Switch `#if defined(PROF)` → `#if PROF`. **CP0, CP0b** (`-DPROF=0` and `-DPROF_ZONE_HISTORY=0` via `CDEFS_EXTRA`).
2. `prof_zone_history_push` in `prof_upd` (throwaway / `record_data` / before `mclr`). **CP1–CP5** at 8, then default **128** and re-run CP4.
3. Accessors + optional title. **CP6, CP10.**

---

## Next

[`prof-step2.md`](prof-step2.md): `PROF_FRAME_HISTORY` — full present (excl+incl+hits, `dt_us`, overhead, totals, `__prof`) for dump / Superluminal. When FRAME is on, drop `PROF_ZONE_EXCL` and read exclusive from the slot.
