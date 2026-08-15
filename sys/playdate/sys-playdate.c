// @per_os_impl Playdate
// Implements the portable sys_* contract for OS_PLAYDATE.
// Private PD / pd_api types stay in this TU (+ sys_playdate.h).

#include "base/mathfunc.h"
#include "base/str.h"
#include "base/dbg.h"

#include "engine/gfx/gfx-defs.h"
#include "lib/tex/tex.h"
#include "sys/sys-debug-draw.h"
#include "sys/sys.h"
#include "base/types.h"
#include "base/log.h"
#include "sys/sys-mem.h"
#include "sys/sys-io.h"
#include "sys/sys-input.h"
#include "sys/playdate/sys-playdate.h"
#include "sys/playdate/sys-playdate-scores.h"
#include "sys/playdate/sys-playdate-scores.c"
#include "base/prof.h"
#include "base/marena.h"
#include "base/utils.h"

PlaydateAPI *PD;

struct pd_menu_item {
	i32 id;
	PDMenuItem *data;
};

struct pd_menu {
	i32 next_id;
	i32 idx;
	i32 len;
	struct pd_menu_item items[3];
};

struct pd_state {
	u32 us_monotonic;
	u32 us_elapsed;
	PDButtons b;
	b32 acc_active;
	u8 keyboard_keys[SYS_KEYS_LEN];
	LCDBitmap *menu_bitmap;
	struct pd_menu menu;
	struct sys_process_info process_info;
};

static struct pd_state PD_STATE;

void (*PD_SYS_LOG_TO_CONSOLE)(const char *fmt, ...);
int (*PD_SYS_PARSE_STR)(const char *str, const char *format, ...);
void *(*PD_SYS_REALLOC)(void *ptr, usize size);

static void (*PD_SYS_GET_BUTTON_STATE)(PDButtons *a, PDButtons *b, PDButtons *c);
static void (*PD_GFX_MARK_UPDATED_ROWS)(int a, int b);
static float (*PD_SYS_GET_CRANK_ANGLE)(void);
static int (*PD_SYS_IS_CRANK_DOCKED)(void);
static float (*PD_SYS_GET_ELAPSED_TIME)(void);
static void (*PD_SYS_RESET_ELAPSED_TIME)(void);
static unsigned int (*PD_SYS_GET_SECONDS_SINCE_EPOCH)(unsigned int *milliseconds);
int (*PD_FILE_WRITE)(SDFile *file, const void *buf, uint len);
int (*PD_FILE_READ)(SDFile *file, void *buf, uint len);

void (*PD_SYS_SET_AUTO_LOCK_DISABLED)(int disable);

int (*PD_SCORE_ADD)(const char *board_id, uint32_t value, AddScoreCallback callback);
void (*PD_SCORE_FREE)(PDScore *score);
int (*PD_SCORES_GET)(const char *board_id, ScoresCallback callback);
void (*PD_SCORES_LIST_FREE)(PDScoresList *scores_list);
void (*PD_SCORE_FREE)(PDScore *score);
int (*PD_PERSONAL_BEST_GET)(const char *board_id, PersonalBestCallback callback);

int sys_pd_update(void *user);
int sys_pd_audio(void *ctx, i16 *lbuf, i16 *rbuf, int len);
static inline u32 sys_pd_sec_to_us(f32 sec);
static void sys_pd_serial_msg(const char *data);

#define PD_SEC_TO_US_MAX 4000u // 4000 * MILLION_U32 stays inside u32 (max ~4295)

int
eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
	switch(event) {
	case kEventInit:
		PD = pd;

		PD_SYS_LOG_TO_CONSOLE          = PD->system->logToConsole;
		PD_SYS_PARSE_STR               = PD->system->parseString;
		PD_SYS_REALLOC                 = PD->system->realloc;
		PD_SYS_GET_ELAPSED_TIME        = PD->system->getElapsedTime;
		PD_SYS_RESET_ELAPSED_TIME      = PD->system->resetElapsedTime;
		PD_SYS_GET_SECONDS_SINCE_EPOCH = PD->system->getSecondsSinceEpoch;
		PD_SYS_GET_BUTTON_STATE        = PD->system->getButtonState;
		PD_SYS_GET_CRANK_ANGLE         = PD->system->getCrankAngle;
		PD_SYS_IS_CRANK_DOCKED         = PD->system->isCrankDocked;
		PD_SYS_SET_AUTO_LOCK_DISABLED  = PD->system->setAutoLockDisabled;

		PD_GFX_MARK_UPDATED_ROWS = PD->graphics->markUpdatedRows;

		PD_FILE_READ  = PD->file->read;
		PD_FILE_WRITE = PD->file->write;

		PD_SCORE_ADD         = PD->scoreboards->addScore;
		PD_SCORE_FREE        = PD->scoreboards->freeScore;
		PD_SCORES_GET        = PD->scoreboards->getScores;
		PD_SCORES_LIST_FREE  = PD->scoreboards->freeScoresList;
		PD_PERSONAL_BEST_GET = PD->scoreboards->getPersonalBest;
		PD_SCORE_FREE        = PD->scoreboards->freeScore;

		PD->system->setUpdateCallback(sys_pd_update, PD);
		PD->sound->addSource(sys_pd_audio, NULL, 0);

		PD->display->setRefreshRate(0.f);
		PD->system->resetElapsedTime();
		PD_STATE.menu_bitmap  = PD->graphics->newBitmap(SYS_DISPLAY_W, SYS_DISPLAY_H, kColorClear);
		PD_STATE.menu.next_id = 1;

		PD_STATE.process_info = (struct sys_process_info){
			.initial_path                  = str8_lit(""),
			.binary_file_path              = str8_lit(""),
			.binary_path                   = str8_lit(""),
			.base_path                     = str8_lit(""),
			.user_program_config_data_path = str8_lit(""),
			.user_program_cache_data_path  = str8_lit(""),
			.user_program_logs_data_path   = str8_lit(""),
		};

		sys_internal_init();
		PD->system->setSerialMessageCallback(sys_pd_serial_msg);
		break;
	case kEventTerminate:
		PD->graphics->freeBitmap(PD_STATE.menu_bitmap);
		sys_internal_close();
		break;
	case kEventPause:
		sys_internal_pause();
		break;
	case kEventResume:
		sys_internal_resume();
		break;
	case kEventKeyPressed: {
		PD_STATE.keyboard_keys[char_to_upper(arg)] = 1;
	} break;
	case kEventKeyReleased: {
		PD_STATE.keyboard_keys[char_to_upper(arg)] = 0;
	} break;
	case kEventMirrorStarted: {
		sys_internal_stream_start();
	} break;
	case kEventMirrorEnded: {
		sys_internal_stream_end();
	} break;

	default: break;
	}
	return 0;
}

f32
sys_pd_crank_deg(void)
{
	return PD->system->getCrankAngle();
}

b32
sys_pd_reduce_flicker(void)
{
	return (b32)PD->system->getReduceFlashing();
}

void
sys_pd_update_rows(i32 from_incl, i32 to_incl)
{
	PD_GFX_MARK_UPDATED_ROWS(from_incl, to_incl);
}

int
sys_pd_update(void *pd)
{
	f32 sec      = PD_SYS_GET_ELAPSED_TIME();
	u32 delta_us = sys_pd_sec_to_us(sec);

	PD_STATE.us_monotonic += delta_us;
	PD_STATE.us_elapsed += delta_us;

	PD_SYS_RESET_ELAPSED_TIME();

	// Playdate: `setRefreshRate(0)` and the OS waits `rows_dirtied/240 * 20ms` (panel ceiling 50 fps).
	// `sys_pd_update` must always `sys_pd_update_rows(0, 239)` and return 1, even when the render gate
	// skips `app_draw`. If a callback dirties nothing / returns 0, the wait is 0, the callback rate
	// rises, and `M` explodes. Do not make draws partial without replacing that wait.
	sys_internal_update();
	sys_pd_update_rows(0, 239);
	return 1;
}

int
sys_pd_audio(void *ctx, i16 *lbuf, i16 *rbuf, int len)
{
	sys_internal_audio(lbuf, rbuf, len);
	return 1;
}

int
sys_inp(void)
{
	PDButtons b;
	PD_SYS_GET_BUTTON_STATE(&b, NULL, NULL);
	return (int)b;
}

int
sys_key(int key)
{
	return PD_STATE.keyboard_keys[key];
}

void
sys_keys(u8 *dest, usize count)
{
	mcpy(dest, PD_STATE.keyboard_keys, sizeof(PD_STATE.keyboard_keys));
}

f32
sys_crank(void)
{
	return (PD_SYS_GET_CRANK_ANGLE() * DEG_TO_TURN);
}

int
sys_crank_docked(void)
{
	return PD_SYS_IS_CRANK_DOCKED();
}

f32
sys_mouse_x(void)
{
	return 0;
}

f32
sys_mouse_y(void)
{
	return 0;
}

f32
sys_time_elapsed(void)
{
	f32 sec     = PD_SYS_GET_ELAPSED_TIME();
	u32 partial = sys_pd_sec_to_us(sec);
	u32 total   = PD_STATE.us_elapsed + partial;
	return (f32)total * 1e-6f;
}

void
sys_time_elapsed_reset(void)
{
	PD_STATE.us_elapsed = 0;
}

// Microseconds
u32
sys_time_us(void)
{
	/*
  https://devforum.play.date/t/get-cycle-count-on-playdate-hardware/10800/2
  The getElapsedTime() function uses the DWT->CYCCNT register to implement a fairly accurate timer:

    float playdate->system->getElapsedTime(void)

    Returns the number of seconds since playdate.resetElapsedTime() was called. The value is a floating-point number with microsecond accuracy.

"microsecond accuracy" there is roughly speaking. Since it returns a floating point value the accuracy depends on how large that value gets, but there's nothing in the code limiting accuracy to 1 uS. Calling resetElapsedTime() before the code you're measuring then getElapsedTime() right after should give you a very accurate measure of execution time, and with some testing you can figure out how much of that is overhead of the reset/getElapsedTime() calls themselves and adjust for that.
*/
	f32 sec     = PD_SYS_GET_ELAPSED_TIME();
	u32 partial = sys_pd_sec_to_us(sec);
	return PD_STATE.us_monotonic + partial;
}

// Milliseconds
u32
sys_time_ms(void)
{
	return sys_time_us() / 1000u;
}

u32
sys_epoch_2000(u32 *milliseconds)
{
	return PD_SYS_GET_SECONDS_SINCE_EPOCH((unsigned int *)milliseconds);
}

void
sys_1bit_invert(b32 value)
{
	PD->display->setInverted(value);
}

void *
sys_1bit_buffer(void)
{
	return PD->graphics->getFrame();
}

v4
sys_color_v4_get(enum gfx_col color)
{
	v4 res = {0};
	switch(color) {
	case GFX_COL_WHITE: {
		res = (v4){1.0f, 1.0f, 1.0f, 1.0f};
	} break;
	case GFX_COL_BLACK: {
		res = (v4){0.0f, 0.0f, 0.0f, 1.0f};
	} break;
	default: {
	} break;
	}
	return res;
}

void
sys_color_v4_set(enum gfx_col color, v4 value)
{
}

u32
sys_color_u32_get(enum gfx_col color)
{
	u32 res = 0;
	switch(color) {
	case GFX_COL_WHITE: {
		res = 0xFFFFFFFF;
	} break;
	case GFX_COL_BLACK: {
		res = 0x000000FF;
	} break;
	default: {
	} break;
	}
	return res;
}

void
sys_color_u32_set(enum gfx_col color, u32 value)
{
}

void *
sys_alloc_raw(ssize size)
{
	return PD_SYS_REALLOC(NULL, size);
}

void
sys_free_raw(void *ptr)
{
	PD_SYS_REALLOC(ptr, 0);
}

void *
sys_alloc(void *ptr, ssize size, ssize align)
{
	return sys_alloc_aligned_raw(size, align, sys_alloc_raw);
}

void
sys_free(void *ptr)
{
	sys_free_aligned_raw(ptr, sys_free_raw);
}

struct alloc
sys_allocator(void)
{
	struct alloc alloc = {
		.allocf = sys_alloc,
		.ctx    = NULL,
	};
	return alloc;
}

void
sys_log(
	const char *tag,
	enum sys_log_level log_level,
	u32 log_item,
	const char *msg,
	uint32_t line_nr,
	const char *filename)
{
	if(log_level > SYS_LOG_LEVEL) { return; }

	const char *log_level_str = NULL;
	switch(log_level) {
	case SYS_LOG_LEVEL_PANI: log_level_str = "PANI"; break;
	case SYS_LOG_LEVEL_ERROR: log_level_str = "ERRO"; break;
	case SYS_LOG_LEVEL_WARN: log_level_str = "WARN"; break;
	default: log_level_str = "INFO"; break;
	}

#if defined(DEV)
	sys_printf("[%s] %s:%d\n %s: %s", log_level_str, filename, (int)line_nr, tag, msg);
#else
	sys_printf("[%s] %s: %s", log_level_str, tag, msg);
#endif
}

struct sys_file_stats
sys_file_stats(str8 path)
{
	FileStat pd_stat = {0};
	int res          = PD->file->stat((const char *)path.str, &pd_stat);
	if(res == -1) {
		log_error("IO", "%s: %s", PD->file->geterr(), path.str);
	}
	return (struct sys_file_stats){
		.isdir    = pd_stat.isdir,
		.size     = pd_stat.size,
		.m_year   = pd_stat.m_year,
		.m_month  = pd_stat.m_month,
		.m_day    = pd_stat.m_day,
		.m_hour   = pd_stat.m_hour,
		.m_minute = pd_stat.m_minute,
		.m_second = pd_stat.m_second,
	};
}

usize
sys_file_modified(str8 path)
{
	struct sys_file_stats stats = sys_file_stats(path);
	usize res                   = (usize)stats.m_year * 10000000000LL +
		(usize)stats.m_month * 100000000 +
		(usize)stats.m_day * 1000000 +
		(usize)stats.m_hour * 10000 +
		(usize)stats.m_minute * 100 +
		(usize)stats.m_second;
	return res;
}

void *
sys_file_open_r(str8 path)
{
	return PD->file->open((char *)path.str, kFileRead | kFileReadData);
}

void *
sys_file_open_w(str8 path)
{
	return PD->file->open((char *)path.str, kFileWrite);
}

void *
sys_file_open_a(str8 path)
{
	return PD->file->open((char *)path.str, kFileAppend);
}

b32
sys_file_close(void *f)
{
	return (PD->file->close(f) == 0);
}

b32
sys_file_del(str8 path)
{
	return (PD->file->unlink((char *)path.str, 1) == 0);
}

b32
sys_file_rename(str8 from, str8 to)
{
	i32 res = PD->file->rename((char *)from.str, (char *)to.str);
	if(res == -1) {
		log_error("io", "failed to rename %s -> %s: %s", from.str, to.str, PD->file->geterr());
	}

	return res == 0;
}

b32
sys_file_flush(void *f)
{
	return (PD->file->flush(f) == 0);
}

i32
sys_file_tell(void *f)
{
	return (i32)PD->file->tell(f);
}

i32
sys_file_seek_set(void *f, i32 pos)
{
	return (i32)PD->file->seek(f, pos, SEEK_SET);
}

i32
sys_file_seek_cur(void *f, i32 pos)
{
	return (i32)PD->file->seek(f, pos, SEEK_CUR);
}

i32
sys_file_seek_end(void *f, i32 pos)
{
	return (i32)PD->file->seek(f, pos, SEEK_END);
}

ssize
sys_file_w(void *f, const void *buf, u32 buf_size)
{
	ssize res = PD_FILE_WRITE(f, buf, (uint)buf_size);
	return res;
}

i32
sys_file_r(void *f, void *buf, u32 buf_size)
{
	return (i32)PD_FILE_READ(f, buf, (uint)buf_size);
}

void
sys_set_auto_lock_disabled(int disable)
{
	PD_SYS_SET_AUTO_LOCK_DISABLED(disable);
}

i32
sys_menu_item_add(const char *title, void (*callback)(void *arg), void *arg)
{
	dbg_assert(PD_STATE.menu.len < (ssize)ARRLEN(PD_STATE.menu.items));
	void *data                = PD->system->addMenuItem(title, callback, arg);
	ssize idx                 = PD_STATE.menu.len++;
	struct pd_menu_item *item = PD_STATE.menu.items + idx;
	item->id                  = PD_STATE.menu.next_id++;
	item->data                = data;
	return item->id;
}

i32
sys_menu_checkmark_add(const char *title, int val, void (*callback)(void *arg), void *arg)
{
	dbg_assert(PD_STATE.menu.len < (ssize)ARRLEN(PD_STATE.menu.items));
	void *data                = PD->system->addCheckmarkMenuItem(title, val, callback, arg);
	ssize idx                 = PD_STATE.menu.len++;
	struct pd_menu_item *item = PD_STATE.menu.items + idx;
	item->id                  = PD_STATE.menu.next_id++;
	item->data                = data;
	return item->id;
}

i32
sys_menu_options_add(const char *title, const char **options, int count, void (*callback)(void *arg), void *arg)
{
	dbg_assert(PD_STATE.menu.len < (ssize)ARRLEN(PD_STATE.menu.items));
	PDMenuItem *data          = PD->system->addOptionsMenuItem(title, options, count, callback, arg);
	ssize idx                 = PD_STATE.menu.len++;
	struct pd_menu_item *item = PD_STATE.menu.items + idx;
	item->id                  = PD_STATE.menu.next_id++;
	item->data                = data;
	return item->id;
}

int
sys_menu_value(int id)
{
	PDMenuItem *data = NULL;

	struct pd_menu_item *items = PD_STATE.menu.items;
	ssize len                  = PD_STATE.menu.len;
	for(ssize i = 0; i < len; i++) {
		if(items[i].id == id) {
			data = items[i].data;
			break;
		}
	}

	return (data ? PD->system->getMenuItemValue(data) : 0);
}

void
sys_menu_item_remove(int id)
{
	struct pd_menu_item *items = PD_STATE.menu.items;
	ssize len                  = PD_STATE.menu.len;

	// Find the index of the item with this id
	ssize idx = -1;
	for(ssize i = 0; i < len; i++) {
		if(items[i].id == id) {
			idx = i;
			if(items[i].data != NULL) {
				PD->system->removeMenuItem(items[i].data);
			}
			break;
		}
	}

	// Not found → nothing to remove (or assert if you prefer)
	if(idx == -1) {
		return;
	}

	// Shift elements left to fill the gap
	for(ssize i = idx; i < len - 1; i++) {
		items[i] = items[i + 1];
	}

	// Clear last element (optional, for safety/debug)
	mclr_struct(&items[len - 1]);

	PD_STATE.menu.len--;
}

void
sys_menu_clr(void)
{
	mclr_array(PD_STATE.menu.items);
	PD_STATE.menu.len = 0;
	PD_STATE.menu.idx = 0;
	PD->system->removeAllMenuItems();
}

void
sys_draw_debug_clear(void)
{
	// LCDBitmap *ctx = PD->graphics->getDebugBitmap();
	// PD->graphics->pushContext(ctx);
	// PD->graphics->popContext();
}

void
sys_debug_draw(struct debug_shape *shapes, int count)
{
#if BUILD_DEBUG && !PD_DEVICE
	LCDBitmap *ctx = PD->graphics->getDebugBitmap();
	PD->graphics->pushContext(ctx);
	for(int i = 0; i < count; ++i) {
		struct debug_shape *shape = &shapes[i];

		switch(shape->type) {
		case DEBUG_CIR: {
			struct debug_shape_cir cir = shape->cir;

			i32 r = cir.d * 0.5;
			int x = cir.p.x - r;
			int y = cir.p.y - r;
			int w = cir.d;
			int h = w;

			if(cir.filled) {
				PD->graphics->fillEllipse(x, y, w, h, 0, 0, kColorWhite);
			} else {
				PD->graphics->drawEllipse(x, y, w, h, 1, 0, 0, kColorWhite);
			}
		} break;
		case DEBUG_REC: {
			struct debug_shape_rec rec = shape->rec;
			int x                      = rec.x;
			int y                      = rec.y;
			int w                      = rec.w;
			int h                      = rec.h;
			if(rec.filled) {
				PD->graphics->fillRect(x, y, w, h, kColorWhite);
			} else {
				PD->graphics->drawRect(x, y, w, h, kColorWhite);
			}
		} break;
		case DEBUG_POLY: {
			struct debug_shape_poly poly = shape->poly;

			for(ssize j = 0; j < poly.count; ++j) {
				v2_i32 a = poly.verts[j];
				v2_i32 b = poly.verts[(j + 1) % poly.count];

				PD->graphics->drawLine(a.x, a.y, b.x, b.y, 1, kColorWhite);
			}

		} break;
		case DEBUG_LIN: {
			struct debug_shape_lin lin = shape->lin;
			PD->graphics->drawLine(lin.a.x, lin.a.y, lin.b.x, lin.b.y, 1, kColorWhite);
		} break;
		case DEBUG_ELLIPSIS: {
			struct debug_shape_ellipsis ellipsis = shape->ellipsis;
			PD->graphics->drawEllipse(
				ellipsis.x - ellipsis.rx,
				ellipsis.y - ellipsis.ry,
				(ellipsis.rx * 2) + 2,
				(ellipsis.ry * 2) + 2,
				1,
				0,
				0,
				kColorWhite);
		} break;
		}
	}
	PD->graphics->popContext();
#endif
}

struct str8
sys_base_path(void)
{
	str8 res = str8_lit("");
	return res;
}

str8
sys_exe_path(void)
{
	str8 res = str8_lit("");
	return res;
}

str8
sys_data_path(void)
{
	str8 res = str8_lit("");
	return res;
}

str8
sys_pref_path(void)
{
	str8 res = str8_lit("");
	return res;
}

void
sys_set_menu_image(struct tex tex, i32 x_offset)
{
	if(tex.px == NULL) {
		PD->system->setMenuImage(NULL, x_offset);
		return;
	}

	int bw, bh, bb;
	u8 *mk = NULL;
	u8 *px = NULL;

	PD->graphics->getBitmapData(
		PD_STATE.menu_bitmap,
		&bw,
		&bh,
		&bb,
		&mk,
		&px);

	if(tex.fmt == TEX_FMT_OPAQUE) {
		if(mk != NULL) {
			mset(mk, 0xFF, bb * bh);
		}
		tex_opaque_to_pdi(tex, px, bw, bh, bb);
	} else {
		tex_mask_to_pdi(tex, px, mk, bw, bh, bb);
	}
	PD->system->setMenuImage(PD_STATE.menu_bitmap, x_offset);
}

void
sys_set_app_name(str8 value)
{
}

struct sys_process_info *
sys_process_info(void)
{
	return &PD_STATE.process_info;
}

str8
sys_get_current_path(struct alloc alloc)
{
	return str8_cpy_push(alloc, str8_lit(""));
}

b32
sys_make_dir(str8 path)
{
	b32 res = PD->file->mkdir((const char *)path.str) == 0;
	return res;
}

static inline u32
sys_pd_sec_to_us(f32 sec)
{
	// (u32)(sec * MILLION_F32) is UB once sec exceeds ~4295
	// Clamp instead of relying on ARM's saturating VCVT.
	if(sec <= 0.0f) { return 0; }
	if(sec >= (f32)PD_SEC_TO_US_MAX) { return PD_SEC_TO_US_MAX * MILLION_U32; }
	return (u32)(sec * MILLION_F32 + 0.5f);
}

static b32
sys_pd_serial_cmd_is(const char *data, const char *cmd)
{
	usize n = strlen(cmd);
	if(strncmp(data, cmd, n) != 0) { return false; }
	char c = data[n];
	return c == 0 || c == '\r' || c == '\n' || c == ' ';
}

#define PD_PROF_CSV_MEM_SIZE MKILOBYTE(512)
static void
sys_pd_serial_msg(const char *data)
{
	if(data == NULL) { return; }
	log_info("pd", "serial message: %s", data);

	if(sys_pd_serial_cmd_is(data, "quit")) {
		PD->system->exitToLauncher();
		return;
	}

	if(sys_pd_serial_cmd_is(data, "prof_save")) {
		void *mem;
		struct marena arena;
		struct alloc alloc;

		mem = sys_alloc_raw(PD_PROF_CSV_MEM_SIZE);
		if(mem == NULL) {
			log_error("prof", "csv save alloc failed");
			return;
		}
		marena_init(&arena, mem, PD_PROF_CSV_MEM_SIZE);
		alloc = marena_allocator(&arena);
		prof_csv_save(alloc, str8_lit("devils-on-the-moon-pinball"), str8_lit("amano"));
		sys_free_raw(mem);
	}
}

// NOLINTBEGIN(readability-identifier-naming)
// make ARM linker shut up about things we aren't using (nosys lib issues):
void
_close(void)
{
}

void
_lseek(void)
{
}

void
_read(void)
{
}

void
_write(void)
{
}

void
_fstat(void)
{
}

void
_getpid(void)
{
}

void
_isatty(void)
{
}

void
_kill(void)
{
}
// NOLINTEND(readability-identifier-naming)
// end ARM linker warning hack
//
//
