#include "pd_api/pd_api_file.h"
#include "sys-types.h"
#include "sys-backend.h"
#include "sys-assert.h"
#include "sys-log.h"
#include "sys-io.h"
#include "sys-input.h"

#include "pd_api.h"
#include <stdarg.h>

PlaydateAPI *PD;

static_assert((int)SYS_FILE_R == (int)(kFileRead | kFileReadData), "file mode");
static_assert((int)SYS_FILE_W == (int)(kFileWrite), "file mode");

static_assert((int)SYS_FILE_SEEK_SET == (int)SEEK_SET, "seek");
static_assert((int)SYS_FILE_SEEK_CUR == (int)SEEK_CUR, "seek");
static_assert((int)SYS_FILE_SEEK_END == (int)SEEK_END, "seek");

static_assert((int)SYS_INP_A == (int)kButtonA, "input button mask A");
static_assert((int)SYS_INP_B == (int)kButtonB, "input button mask B");
static_assert((int)SYS_INP_DPAD_L == (int)kButtonLeft, "input button mask Dpad left");
static_assert((int)SYS_INP_DPAD_R == (int)kButtonRight, "input button mask Dpad right");
static_assert((int)SYS_INP_DPAD_U == (int)kButtonUp, "input button mask Dpad up");
static_assert((int)SYS_INP_DPAD_D == (int)kButtonDown, "input button mask Dpad down");

int (*PD_PARSE_STR)(const char *str, const char *format, ...);
void *(*PD_REALLOC)(void *ptr, usize size);

static void (*PD_GET_BUTTON_STATE)(PDButtons *a, PDButtons *b, PDButtons *c);
static void (*PD_MARK_UPDATED_ROWS)(int a, int b);
static float (*PD_GET_CRANK_ANGLE)(void);
static int (*PD_IS_CRANK_DOCKED)(void);
static float (*PD_SECONDS)(void);

static u8 KEYS[SYS_KEYS_LEN];

void (*PD_LOG)(const char *fmt, ...);

int
eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
	switch(event) {
	case kEventInit:
		PD = pd;

		PD_LOG               = PD->system->logToConsole;
		PD_PARSE_STR         = PD->system->parseString;
		PD_REALLOC           = PD->system->realloc;
		PD_MARK_UPDATED_ROWS = PD->graphics->markUpdatedRows;
		PD_SECONDS           = PD->system->getElapsedTime;

		PD_GET_BUTTON_STATE = PD->system->getButtonState;
		PD_GET_CRANK_ANGLE  = PD->system->getCrankAngle;
		PD_IS_CRANK_DOCKED  = PD->system->isCrankDocked;

		PD->system->setUpdateCallback(sys_tick, PD);
		PD->display->setRefreshRate(0.f);
		PD->system->resetElapsedTime();
		sys_init();
		break;
	case kEventTerminate:
		sys_close();
		break;
	case kEventPause:
		sys_pause();
		break;
	case kEventResume:
		sys_resume();
		break;
	case kEventKeyPressed: {
		KEYS[arg] = 1;
	} break;
	case kEventKeyReleased: {
		KEYS[arg] = 0;
	} break;
	case kEventInitLua:
	case kEventLock:
	case kEventUnlock:
	case kEventLowPower: break;
	}
	return 0;
}

int
backend_inp(void)
{
	PDButtons b;
	PD_GET_BUTTON_STATE(&b, NULL, NULL);
	return (int)b;
}

int
backend_key(int key)
{
	return KEYS[key];
}

u8 *
backend_keys(void)
{
	return KEYS;
}

f32
backend_crank(void)
{
	return (PD_GET_CRANK_ANGLE() * 0.002777778f); // DEG_TO_TURN
}

int
backend_crank_docked(void)
{
	return PD_IS_CRANK_DOCKED();
}

void
backend_display_row_updated(int a, int b)
{
	PD_MARK_UPDATED_ROWS(a, b);
}

f32
backend_seconds(void)
{
	return PD_SECONDS();
}

u32 *
backend_framebuffer(void)
{
	return (u32 *)PD->graphics->getFrame();
}

void *
backend_alloc(void *ptr, usize size)
{
	return PD_REALLOC(ptr, size);
}

void
backend_free(void *ptr)
{
	PD_REALLOC(ptr, 0);
}

void
backend_log(const char *tag, u32 log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename)
{
	const char *log_level_str;
	switch(log_level) {
	case 0: log_level_str = "PANI"; break;
	case 1: log_level_str = "ERRO"; break;
	case 2: log_level_str = "WARN"; break;
	default: log_level_str = "INFO"; break;
	}

	char strret[1024] = {0};
	stbsp_sprintf(strret,
		"[%s] %s: %s",
		log_level_str,
		tag,
		msg);

	PD_LOG(strret);
}

struct sys_file_stats
backend_file_stats(const char *path)
{
	FileStat pd_stat = {0};
	int res          = PD->file->stat(path, &pd_stat);
	if(res == -1) {
		log_error("IO", "%s: %s", PD->file->geterr(), path);
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

void *
backend_file_open(const char *path, int mode)
{
	void *r = (void *)PD->file->open(path, mode);
	if(r == NULL) {
		log_error("IO", "%s: %s", PD->file->geterr(), path);
	}

	return r;
}

int
backend_file_close(void *f)
{
	int r = PD->file->close((SDFile *)f);
	return r;
}

int
backend_file_flush(void *f)
{
	int r = PD->file->flush((SDFile *)f);
	return r;
}

int
backend_file_read(void *f, void *buf, usize buf_size)
{
	int r = PD->file->read((SDFile *)f, buf, buf_size);
	return r;
}

int
backend_file_write(void *f, const void *buf, usize buf_size)
{
	int r = PD->file->write((SDFile *)f, buf, buf_size);
	return r;
}

int
backend_file_tell(void *f)
{
	int r = PD->file->tell((SDFile *)f);
	return r;
}

int
backend_file_seek(void *f, int pos, int origin)
{
	int r = PD->file->seek((SDFile *)f, pos, origin);
	return r;
}

int
backend_file_remove(const char *path)
{
	int r = PD->file->unlink(path, 0);
	return r;
}

void *
backend_menu_item_add(const char *title, void (*callback)(void *arg), void *arg)
{
	PDMenuItem *mi = PD->system->addMenuItem(title, callback, arg);
	return mi;
}

void *
backend_menu_checkmark_add(const char *title, int val, void (*callback)(void *arg), void *arg)
{
	PDMenuItem *mi = PD->system->addCheckmarkMenuItem(title, val, callback, arg);
	return mi;
}

void *
backend_menu_options_add(const char *title, const char **options, int count, void (*callback)(void *arg), void *arg)
{
	PDMenuItem *mi = PD->system->addOptionsMenuItem(title, options, count, callback, arg);
	return mi;
}

int
backend_menu_value(void *ptr)
{
	return (ptr ? PD->system->getMenuItemValue((PDMenuItem *)ptr) : 0);
}

void
backend_menu_clr(void)
{
	PD->system->removeAllMenuItems();
}

int
backend_debug_space(void)
{
	return 0;
}

void
backend_draw_debug_clear(void)
{

	LCDBitmap *ctx = PD->graphics->getDebugBitmap();
	PD->graphics->pushContext(ctx);

	PD->graphics->popContext();
}

void
backend_draw_debug_shape(struct debug_shape *shapes, int count)
{
#if !defined(TARGET_PLAYDATE)
	LCDBitmap *ctx = PD->graphics->getDebugBitmap();
	PD->graphics->pushContext(ctx);
	for(int i = 0; i < count; ++i) {
		struct debug_shape *shape = &shapes[i];

		switch(shape->type) {
		case DEBUG_CIR: {
			struct debug_shape_cir cir = shape->cir;

			int x = cir.p.x - cir.r;
			int y = cir.p.y - cir.r;
			int w = cir.r + cir.r;
			int h = w;

			PD->graphics->drawEllipse(x, y, h, w, 1, 0, 0, kColorWhite);
		} break;
		case DEBUG_REC: {
			struct debug_shape_rec rec = shape->rec;
			int x                      = rec.x;
			int y                      = rec.y;
			int w                      = rec.w;
			int h                      = rec.h;
			PD->graphics->drawRect(x, y, w, h, kColorWhite);
		} break;
		case DEBUG_POLY: {
			struct debug_shape_poly poly = shape->poly;

			for(int i = 0; i < poly.count; ++i) {
				v2_i32 a = poly.verts[i];
				v2_i32 b = poly.verts[(i + 1) % poly.count];

				PD->graphics->drawLine(a.x, a.y, b.x, b.y, 1, kColorWhite);
			}

		} break;
		case DEBUG_LIN: {
			struct debug_shape_lin lin = shape->lin;
			PD->graphics->drawLine(lin.a.x, lin.a.y, lin.b.x, lin.b.y, 1, kColorWhite);
		} break;
		}
	}
	PD->graphics->popContext();
#endif
}

struct exe_path
backend_where(void)
{
	return (struct exe_path){0};
}
