# Step 2: `PROF_FRAME_HISTORY`

Spec only. Full present ring, like iProf `Prof_CALL_HISTORY`, plus `dt_us`, profiler overhead, capture-wide totals, and `__prof`. For dump / Superluminal comparison later — do not implement binary dump, `luna-prof-csv`, device script, or Superluminal wiring here.

Depends on [`prof-step1.md`](prof-step1.md): wrap, pause, throwaway, and `PROF_HISTORY_SIZE` already work for `PROF_ZONE_HISTORY`. This step adds the heavier ring and extra timers.

Scope is [`luna/base/prof.h`](prof.h) only. Do not change `sys.c` call sites.

## Two knobs

Same defaults as [step 1](prof-step1.md). Debug (`BUILD_DEBUG=1`) turns **PROF, ZONE, and FRAME** on unless a knob is passed as `0`. This step **honors** `PROF_FRAME_HISTORY` (step 1 left it unused).

```c
#ifndef PROF
#define PROF BUILD_DEBUG
#endif
#ifndef PROF_ZONE_HISTORY
#define PROF_ZONE_HISTORY PROF
#endif
#ifndef PROF_FRAME_HISTORY
#define PROF_FRAME_HISTORY PROF
#endif
#ifndef PROF_HISTORY_SIZE
#define PROF_HISTORY_SIZE 128       // iprof; step 1 already moved off 8
#endif
```

Use `#if PROF && PROF_FRAME_HISTORY`, not `#if defined(PROF_FRAME_HISTORY)` (`-DPROF_FRAME_HISTORY=0` is still defined).

| Build | Storage | Extra start/end timers |
| --- | --- | --- |
| `PROF=0` (release default) | none | no |
| `PROF=1`, both history `=0` | none | no |
| `PROF=1` + ZONE only | `u32 excl[96][N]` (~3 KB / ~48 KB) | no |
| `PROF=1` + FRAME (debug default) | `prof_hist_slot[N]` (~9 KB / ~145 KB) + totals (~2.3 KB) | yes |
| ZONE + FRAME both 1 | **FRAME only** — do not also allocate `PROF_ZONE_EXCL`. Graph / `prof_zone_excl_at` read `slot->anchors[i].us_exclusive`. | yes |

`PROF=1 PROF_FRAME_HISTORY=0 PROF_ZONE_HISTORY=1` is the cheap exclusive ring. `PROF_FRAME_HISTORY=1` is enough for Superluminal compare. ZONE alone is not (no incl/hits/`dt`/overhead).

FRAME without `PROF` does nothing. `PROF_HISTORY_SIZE` ignored when `PROF` is 0.

Makefile forwarding is already how `SYS_LOG_LEVEL` works (`CDEFS` / `CDEFS_EXTRA="-DPROF_FRAME_HISTORY=0"`). No new makefile knobs.

Compile-time `#if`. No runtime skip-copy. One push path for all `N >= 1`.

## Goals

1. When `PROF_FRAME_HISTORY` is on, keep a **ring of the last N presents**: raw `us_exclusive` / `us_inclusive` / `hit_count` for every zone, plus `dt_us` and this-present profiler overhead.
2. Capture-wide **totals** so a long run still has a true average after the ring wraps.
3. Measure **profiler cost** without putting it on the zone stack (no `prof_block` around `prof_block`). Extra timers **only** when FRAME is on.
4. `__prof` row on overlay / `prof_csv` when FRAME is on.
5. Do **not** subtract overhead from zone times. Record it.
6. ZONE-only (`FRAME=0`) and overlay-only stay as after step 1 (no FRAME BSS, no extra timers).

## Non-goals

- Binary `.prof`, `luna-prof-csv`, `PROF_CAPTURE_S`, `profile-playdate.sh`
- Changing `sys_time_us()` / overlay draw/skip
- Drawing the graph (ZONE ring or FRAME exclusive series is enough later)

---

## Memory

| Item | Size |
| --- | --- |
| `struct prof_anchor` | 12 bytes |
| anchors per slot | `96 * 12 = 1152` |
| `struct prof_hist_slot` | `8 + 1152 = 1160` |
| FRAME ring | `N * 1160` (**~9 KB** at 8; **~145 KB** at 128) |
| `totals[]` | `96 * 24 = 2304` |
| ZONE array when FRAME on | **0** (superseded) |

Do **not** embed the ring in `struct prof`. File-static BSS. 145 KB fits Playdate `SYS_MAX_MEM` (14 MB) with no change.

ZONE vs FRAME (96 zones, N=128): ~48 KB / ~145 KB. Same three `u32`s per zone as iProf `Prof_CALL_HISTORY` (~144 KB); this slot adds 8 B/present for `dt_us`/`overhead_us`. Layout is per-present (dump), not per-zone (graph).

---

## Data layout

```c
#if PROF && PROF_FRAME_HISTORY
struct prof_hist_slot {
	u32 dt_us;
	u32 overhead_us; // block + upd for this present only
	struct prof_anchor anchors[PROF_ANCHORS_SIZE];
};

struct prof_anchor_tot {
	u64 sum_exc;
	u64 sum_inc;
	u64 sum_hits;
};
#endif

struct prof {
	/* existing + step 1 zone fields ... */

#if PROF && PROF_FRAME_HISTORY
	struct prof_hist_slot *history; // -> PROF_FRAME_STORAGE
	u32 capture_n;
	struct prof_anchor_tot totals[PROF_ANCHORS_SIZE];
	u32 overhead_block_us;
	u32 overhead_upd_us;
	u64 overhead_block_sum_us;
	u64 overhead_upd_sum_us;
	u32 timer_call_us;
#endif
};

#if PROF && PROF_FRAME_HISTORY
static struct prof_hist_slot PROF_FRAME_STORAGE[PROF_HISTORY_SIZE];
#endif

#if PROF && PROF_ZONE_HISTORY && !PROF_FRAME_HISTORY
static u32 PROF_ZONE_EXCL[PROF_ANCHORS_SIZE][PROF_HISTORY_SIZE];
#endif
```

Share `history_idx` / `history_filled` between ZONE and FRAME (one present index). If step 1 put them under `#if PROF && PROF_ZONE_HISTORY` only, widen to:

```c
#if PROF && (PROF_ZONE_HISTORY || PROF_FRAME_HISTORY)
	u16 history_idx;
	u16 history_filled;
#endif
```

`prof_ini` when FRAME: `prof->history = PROF_FRAME_STORAGE`, calibrate `timer_call_us`. No `mclr` of the ring at init.

---

## Timer calibration

Only when `PROF_FRAME_HISTORY`. Once in `prof_ini`, not on the frame path.

```c
static inline u32
prof_calibrate_timer_us(void)
{
	enum { N = 1000 };
	u32 t0 = sys_time_us();
	for(i32 i = 0; i < N; ++i) {
		(void)sys_time_us();
	}
	u32 t1 = sys_time_us();
	return (t1 - t0) / (u32)N;
}
```

Expect 0–2 µs on Linux if `sys_time_us` quantizes. Store in `prof->timer_call_us`.

---

## Overhead: do not use the zone stack

Whole section is `#if PROF && PROF_FRAME_HISTORY`. ZONE-only and PROF-only keep today’s timer shape (1 / 1 / 1).

Do not add `PROF_ANCHOR_SYS_PROF` or wrap start/end in `prof_block`.

### `prof_block_start`

Keep `us_start` after bookkeeping (child exclusive = game work), same as today.

```c
static inline void
prof_block_start(const char *label, ssize idx)
{
#if PROF && PROF_FRAME_HISTORY
	u32 t0 = sys_time_us();
#endif
	struct prof *prof = &PROFILER;
	dbg_assert(idx >= 0 && idx < (ssize)ARRLEN(prof->anchors));
	dbg_assert(prof->frame_count < (ssize)ARRLEN(prof->frames));

	prof->anchor_count       = MAX(idx + 1, prof->anchor_count);
	struct prof_anchor *item = prof->anchors + idx;
	u32 zone_start           = sys_time_us();
	prof->frames[prof->frame_count++] = (struct prof_frame){
		.prev_us_inclusive = item->us_inclusive,
		.us_start          = zone_start,
		.parent_idx        = prof->parent_idx,
		.anchor_idx        = idx,
		.label             = label,
	};
	prof->parent_idx = idx;

#if PROF && PROF_FRAME_HISTORY
	u32 t1 = sys_time_us();
	prof->overhead_block_us += (t1 - t0);
#endif
}
```

FRAME start uses **3** timer calls. The steal from `zone_start` is real profiler cost. Do not subtract it from the zone.

### `prof_block_end_internal`

`now_us` first (end bookkeeping not inside the child). Charge the rest to overhead.

```c
	/* existing end math */

#if PROF && PROF_FRAME_HISTORY
	u32 t1 = sys_time_us();
	prof->overhead_block_us += (t1 - now_us);
#endif
```

Do not subtract `overhead_block_us` from `upd`/`drw`.

### `prof_upd`

```c
static inline void
prof_upd(b32 record_data)
{
#if PROF && PROF_FRAME_HISTORY
	u32 t0 = sys_time_us();
#endif
	struct prof *prof = &PROFILER;
	dbg_assert(prof->frame_count == 0);
	dbg_assert(prof->parent_idx == 0);

	/* existing dt_us / EMA-factor setup; if FRAME, t0 is now_us */

	if(record_data) {
		if(prof->update_idx >= PROF_THROWAWAY_UPDATES_COUNT) {
#if PROF && PROF_FRAME_HISTORY
			prof_frame_history_push(prof, dt_us);
			prof_totals_add(prof);
#elif PROF && PROF_ZONE_HISTORY
			prof_zone_history_push(prof);
#endif
		}
		/* existing EMA / present_per_s / throwaway */
		++prof->update_idx;
	}

	if(prof->anchor_count > 0) {
		mclr(prof->anchors + 1, (prof->anchor_count - 1) * sizeof(prof->anchors[0]));
	}

#if PROF && PROF_FRAME_HISTORY
	u32 t1 = sys_time_us();
	prof->overhead_upd_us = (t1 - t0);
	if(record_data && prof->update_idx > PROF_THROWAWAY_UPDATES_COUNT) {
		prof->overhead_upd_sum_us += prof->overhead_upd_us;
		u16 just = (u16)((prof->history_idx + PROF_HISTORY_SIZE - 1) % PROF_HISTORY_SIZE);
		prof->history[just].overhead_us += prof->overhead_upd_us;
		prof->overhead_block_sum_us += /* block us already in that slot */;
	}
	prof->overhead_block_us = 0;
	prof->overhead_upd_us   = 0;
#endif
}
```

The `#elif PROF_ZONE_HISTORY` keeps step 1 working when FRAME is off. When FRAME is on, do **not** also call `prof_zone_history_push`.

Order when FRAME is on:

1. `overhead_block_us` already filled by start/end.
2. Copy slot + totals **before** `mclr`.
3. EMA unchanged.
4. Clear anchors.
5. Then measure `overhead_upd_us` and add it onto the slot just written.

```c
#if PROF && PROF_FRAME_HISTORY
static inline void
prof_frame_history_push(struct prof *prof, u32 dt_us)
{
	struct prof_hist_slot *s = &prof->history[prof->history_idx];
	s->dt_us                 = dt_us;
	s->overhead_us           = prof->overhead_block_us; // upd added later
	mcpy(s->anchors, prof->anchors, sizeof(s->anchors));

	prof->history_idx = (u16)((prof->history_idx + 1) % PROF_HISTORY_SIZE);
	if(prof->history_filled < PROF_HISTORY_SIZE) {
		++prof->history_filled;
	}
	++prof->capture_n;
}
#endif
```

Pause freezes the ring (`record_data` false). Clear `overhead_block_us` at end of every `prof_upd` so pause does not accumulate seconds of `__prof`.

---

## Capture-wide totals

Only when FRAME is on.

```c
static inline void
prof_totals_add(struct prof *prof)
{
	for(ssize i = 1; i < prof->anchor_count; ++i) {
		struct prof_anchor *a     = &prof->anchors[i];
		struct prof_anchor_tot *t = &prof->totals[i];
		t->sum_exc += a->us_exclusive;
		t->sum_inc += a->us_inclusive;
		t->sum_hits += a->hit_count;
	}
}
```

Do **not** reset totals on wrap. `capture_n` is `u32`. Throwaway skips ring and totals.

---

## Ring read order

Same logical-oldest rule as step 1.

```c
static inline struct prof_hist_slot *
prof_history_logical(struct prof *prof, u16 logical_i)
{
	dbg_assert(logical_i < prof->history_filled);
	u16 oldest = 0;
	if(prof->history_filled == PROF_HISTORY_SIZE) {
		oldest = prof->history_idx;
	}
	u16 i = (u16)((oldest + logical_i) % PROF_HISTORY_SIZE);
	return &prof->history[i];
}
```

`prof_zone_excl_at(zone, logical_i)`: if FRAME, return `prof_history_logical(...)->anchors[zone].us_exclusive`; else step 1’s `PROF_ZONE_EXCL`.

---

## Report and CSV: `__prof` row

Only when FRAME is on and `capture_n > 0`. ZONE-only: no extra row (step 1).

```c
if(prof->capture_n > 0) {
	struct prof_report_entry *e = &res->entries[res->entry_count++];
	e->label                    = "__prof";
	e->ms_exclusive             = (f32)(prof->overhead_block_sum_us + prof->overhead_upd_sum_us)
		/ (f32)prof->capture_n * 1e-3f;
	e->ms_inclusive = e->ms_exclusive;
	e->hit_count    = 1;
	e->ms_inclusive_min = 0;
	e->ms_inclusive_max = 0;
}

res->titles[1] = str8_fmt_push(alloc, "prof %sus  timer %sus",
	/* last present or capture-avg overhead */,
	prof->timer_call_us);
```

Optional `titles[2]`: `hist %u/%u  n %u` → `filled/SIZE` and `capture_n`. `prof_csv` already prints every report row.

---

## Accessors

Step 1 accessors stay. Add (stubs when `!PROF` or `!PROF_FRAME_HISTORY`):

```c
u32  prof_capture_n(void);
u32  prof_timer_call_us(void);
u64  prof_overhead_block_sum_us(void);
u64  prof_overhead_upd_sum_us(void);
const struct prof_hist_slot *prof_history_logical_at(u16 logical_i);
```

`prof_history_capacity()`: `SIZE` if ZONE or FRAME, else 0. Dump tools: `prof_history_logical_at` NULL means no FRAME ring (ZONE-only or off). `capacity == 1` is a one-slot ring, not off.

---

## Invariants

- Step 1 invariants still hold for ZONE-only.
- With FRAME: extra timer steal on zones; children must still be `<=` parent inclusive.
- `mclr` every present.
- EMA unchanged aside from `__prof` / titles when FRAME is on.

---

## Checkpoints

Linux debug default (`linux_dev_build`) now has **FRAME on**. Overlay-only / ZONE-only need `PROF_FRAME_HISTORY=0`. Re-use step 1 wrap tests; SIZE should already be 128.

### CP0 — compiles

- Debug default: `PROF_FRAME_STORAGE` ≈ 145 KB at 128; **no** `PROF_ZONE_EXCL`.
- `PROF_FRAME_HISTORY=0 PROF_ZONE_HISTORY=1`: `PROF_ZONE_EXCL` present, **no** `PROF_FRAME_STORAGE`.
- `PROF=1` both history `=0`: neither array.
- `PROF=0` (or `BUILD_DEBUG=0`): neither array, even if `PROF_FRAME_HISTORY=1`.
- `PROF_ZONE_HISTORY=1 PROF_FRAME_HISTORY=1`: FRAME storage only.

**Fail if:** `CDEFS_EXTRA="-DPROF_FRAME_HISTORY=0"` still allocates `PROF_FRAME_STORAGE` (`defined()` bug).

### CP0b — PROF / ZONE-only unchanged

`PROF_FRAME_HISTORY=0`: no extra start/end timers, no `__prof`. With `PROF_ZONE_HISTORY=1`, exclusive ring still works (step 1). Nested exclusive matches step 1.

### CP1–CP5 — FRAME ring

Same as step 1 but inspect `s->anchors[UPD].us_exclusive` after `mcpy`. `capture_n` rises past `SIZE` on wrap; `filled` saturates. Pause freezes `filled` and `capture_n`.

### CP6 — zone math with extra timers

FRAME on, `PROF_SMOOTH_INSTANT`: last slot excl/incl/count match live values before `mclr`. Children not systematically larger than parent inclusive.

### CP7 — totals vs ring

Before wrap: `totals[UPD].sum_exc / capture_n` ≈ mean of slot exclusives. After wrap: totals = full window; ring = last N only.

### CP8 — `__prof`

`timer_call_us` stable. `__prof` exclusive > 0. Busier present → that slot’s `overhead_us` up; `overhead_upd_us` roughly flat.

**Fail if:** always 0, or `__prof` ≈ frame time (`t0` must be the start of `prof_upd` only).

### CP9 — pause does not explode overhead

Pause seconds, resume one present: next `__prof` / slot `overhead_us` is a normal per-present value.

### CP10 — `PROF_PRINT`

ZONE-only: no `__prof`. FRAME: `__prof` row + optional `hist filled/SIZE n=capture_n`. CSV exclusive is still EMA.

### CP11 — `PROF` off

No-ops. No FRAME / ZONE BSS.

---

## Suggested implementation order

1. Widen idx/filled; FRAME structs/storage; `#if PROF && PROF_ZONE_HISTORY && !PROF_FRAME_HISTORY` around `PROF_ZONE_EXCL`. **CP0, CP0b.**
2. `prof_frame_history_push` + `#elif` ZONE push. **CP1–CP5.**
3. Totals. **CP7.**
4. Overhead brackets (FRAME `#if` only). **CP8–CP9.**
5. `__prof` + titles. **CP10.**
6. Accessors; `prof_zone_excl_at` reads slots when FRAME. **CP6.**

---

## Later (out of scope)

Binary dump, `luna-prof-csv`, Playdate script, Superluminal compare. They **read** FRAME slots, totals, and overhead via the accessors. ZONE-only dumps are not comparable to Superluminal (exclusive sparkline only).
