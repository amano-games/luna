
#include "mathfunc.h"
#include "str.h"
#include "sys-debug.h"
#include "sys.h"
#include "sys-types.h"
#include "sys-log.h"
#include "sys-io.h"
#include "sys-input.h"
#include "sys-assert.h"

PlaydateAPI *PD;
struct pd_state {
	PDButtons b;
	bool32 acc_active;
	u8 keyboard_keys[SYS_KEYS_LEN];
	LCDBitmap *menu_bitmap;
	PDMenuItem *menu_items[5];
};

static struct pd_state PD_STATE;

int (*PD_SYSTEM_PARSE_STR)(const char *str, const char *format, ...);
void *(*PD_SYSTEM_REALLOC)(void *ptr, usize size);
void (*PD_SYSTEM_LOG_TO_CONSOLE)(const char *fmt, ...);

static void (*PD_SYSTEM_GET_BUTTON_STATE)(PDButtons *a, PDButtons *b, PDButtons *c);
static void (*PD_GRAPHICS_MARK_UPDATED_ROWS)(int a, int b);
static float (*PD_SYSTEM_GET_CRANK_ANGLE)(void);
static int (*PD_SYSTEM_IS_CRANK_DOCKED)(void);
static float (*PD_SYSTEM_GET_ELAPSED_TIME)(void);
static unsigned int (*PD_SYSTEM_GET_SECONDS_SINCE_EPOCH)(unsigned int *milliseconds);
int (*PD_FILE_WRITE)(SDFile *file, const void *buf, uint len);
int (*PD_FILE_READ)(SDFile *file, void *buf, uint len);

int sys_pd_update(void *user);
int sys_pd_audio(void *ctx, i16 *lbuf, i16 *rbuf, int len);

int
eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
	switch(event) {
	case kEventInit:
		PD = pd;

		PD_SYSTEM_LOG_TO_CONSOLE          = PD->system->logToConsole;
		PD_SYSTEM_PARSE_STR               = PD->system->parseString;
		PD_SYSTEM_REALLOC                 = PD->system->realloc;
		PD_GRAPHICS_MARK_UPDATED_ROWS     = PD->graphics->markUpdatedRows;
		PD_SYSTEM_GET_ELAPSED_TIME        = PD->system->getElapsedTime;
		PD_SYSTEM_GET_SECONDS_SINCE_EPOCH = PD->system->getSecondsSinceEpoch;
		PD_SYSTEM_GET_BUTTON_STATE        = PD->system->getButtonState;
		PD_SYSTEM_GET_CRANK_ANGLE         = PD->system->getCrankAngle;
		PD_SYSTEM_IS_CRANK_DOCKED         = PD->system->isCrankDocked;
		PD_FILE_READ                      = PD->file->read;
		PD_FILE_WRITE                     = PD->file->write;

		PD->system->setUpdateCallback(sys_pd_update, PD);
		PD->sound->addSource(sys_pd_audio, NULL, 0);

		PD->display->setRefreshRate(0.f);
		PD->system->resetElapsedTime();
		PD_STATE.menu_bitmap = PD->graphics->newBitmap(SYS_DISPLAY_W, SYS_DISPLAY_H, kColorBlack);
		{
			PD->graphics->pushContext(PD_STATE.menu_bitmap);
			PD->graphics->fillEllipse(0, 0, 30, 30, 0, 0, kColorWhite);
			PD->graphics->popContext();
			int *width    = NULL;
			int *height   = NULL;
			int *rowbytes = NULL;
			u8 **mask     = NULL;
			u8 **data     = NULL;

			PD->graphics->getBitmapData(
				PD_STATE.menu_bitmap,
				width,
				height,
				rowbytes,
				mask,
				data);
			sys_printf("hey");
		}

		sys_internal_init();
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

	default: break;
	}
	return 0;
}

f32
sys_pd_crank_deg(void)
{
	return PD->system->getCrankAngle();
}

bool32
sys_pd_reduce_flicker(void)
{
	return (bool32)PD->system->getReduceFlashing();
}

void
sys_pd_update_rows(i32 from_incl, i32 to_incl)
{
	PD_GRAPHICS_MARK_UPDATED_ROWS(from_incl, to_incl);
}

int
sys_pd_update(void *pd)
{
	return sys_internal_update();
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
	PD_SYSTEM_GET_BUTTON_STATE(&b, NULL, NULL);
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
	return (PD_SYSTEM_GET_CRANK_ANGLE() * DEG_TO_TURN);
}

int
sys_crank_docked(void)
{
	return PD_SYSTEM_IS_CRANK_DOCKED();
}

f32
sys_seconds(void)
{
	return PD_SYSTEM_GET_ELAPSED_TIME();
}

u32
sys_epoch(u32 *milliseconds)
{
	return PD_SYSTEM_GET_SECONDS_SINCE_EPOCH((unsigned int *)milliseconds);
}

void
sys_1bit_invert(bool32 i)
{
	PD->display->setInverted(i);
}

void *
sys_1bit_buffer(void)
{
	return PD->graphics->getFrame();
}

void *
sys_1bit_menu_buffer(void)
{
	int *width    = NULL;
	int *height   = NULL;
	int *rowbytes = NULL;
	u8 **mask     = NULL;
	u8 **data     = NULL;

	PD->graphics->getBitmapData(
		PD_STATE.menu_bitmap,
		width,
		height,
		rowbytes,
		mask,
		data);
	return data;
}

void *
sys_alloc(void *ptr, usize size)
{
	return PD_SYSTEM_REALLOC(ptr, size);
}

void
sys_free(void *ptr)
{
	PD_SYSTEM_REALLOC(ptr, 0);
}

void
sys_log(const char *tag, u32 log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename)
{
	if(log_level <= SYS_LOG_LEVEL) {
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

		PD_SYSTEM_LOG_TO_CONSOLE(strret);
	}
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

bool32
sys_file_close(void *f)
{
	return (PD->file->close(f) == 0);
}

bool32
sys_file_del(str8 path)
{
	return (PD->file->unlink((char *)path.str, 1) == 0);
}

bool32
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

i32
sys_file_w(void *f, const void *buf, u32 buf_size)
{
	return (i32)PD_FILE_WRITE(f, buf, (uint)buf_size);
}

i32
sys_file_r(void *f, void *buf, u32 buf_size)
{
	return (i32)PD_FILE_READ(f, buf, (uint)buf_size);
}

void
sys_menu_item_add(int id, const char *title, void (*callback)(void *arg), void *arg)
{
	void *item              = PD->system->addMenuItem(title, callback, arg);
	PD_STATE.menu_items[id] = item;
}

void
sys_menu_checkmark_add(int id, const char *title, int val, void (*callback)(void *arg), void *arg)
{
	void *item              = PD->system->addCheckmarkMenuItem(title, val, callback, arg);
	PD_STATE.menu_items[id] = item;
}

void
sys_menu_options_add(int id, const char *title, const char **options, int count, void (*callback)(void *arg), void *arg)
{
	PDMenuItem *item        = PD->system->addOptionsMenuItem(title, options, count, callback, arg);
	PD_STATE.menu_items[id] = item;
}

int
sys_menu_value(int id)
{
	PDMenuItem *ptr = PD_STATE.menu_items[id];
	return (ptr ? PD->system->getMenuItemValue(ptr) : 0);
}

void
sys_menu_clr(void)
{
	mclr(PD_STATE.menu_items, sizeof(PD_STATE.menu_items));
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
#if !defined(TARGET_PLAYDATE)
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

str8
sys_where(struct alloc alloc)
{
	str8 res = str8_lit("/");
	return res;
}

void
sys_set_menu_image(void *px, int h, int wbyte, i32 x_offset)
{
	if(px == NULL) {
		PD->system->setMenuImage(NULL, x_offset);
		return;
	}
	int wid, hei, byt;
	u8 *p;
	PD->graphics->getBitmapData(PD_STATE.menu_bitmap, &wid, &hei, &byt, NULL, &p);
	int y2 = hei < h ? hei : h;
	int b2 = byt < wbyte ? byt : wbyte;
	for(int y = 0; y < y2; y++) {
		for(int b = 0; b < b2; b++)
			p[b + y * byt] = ((u8 *)px)[b + y * wbyte];
	}
	PD->system->setMenuImage(PD_STATE.menu_bitmap, x_offset);
}
