#include "sys/sys-log.h"
#include "sys/sys-io.h"
#include "sys/sys-os.h"
#include "sys/sys-printf.h"

#define SYS_LOG_LINE_LENGTH (512) // parity with sokol_log.h _SLOG_LINE_LENGTH

static sys_file SYS_LOG_FILE_HANDLE = {0};

b32
sys_log_ini(struct alloc alloc, str8 path)
{
	b32 res = false;
	if(SYS_LOG_FILE) {
		if(sys_file_is_valid(SYS_LOG_FILE_HANDLE)) { return true; }
		if(path.size == 0) { return false; }

		str8 dir = str8_chop_last_slash(path);
		if(dir.size > 0) { sys_make_dirs(dir); }

		SYS_LOG_FILE_HANDLE = sys_file_open_w(path);
		res                 = sys_file_is_valid(SYS_LOG_FILE_HANDLE);
		if(res) {
			log_info("sys-log", "log file path: %s", path);
		} else {
			log_warn("sys-log", "failed to open log file path: %s", path);
		}
	}

	return res;
}

void
sys_log_close(void)
{
	if(sys_file_is_valid(SYS_LOG_FILE_HANDLE)) {
		sys_file_flush(SYS_LOG_FILE_HANDLE);
		sys_file_close(SYS_LOG_FILE_HANDLE);
		SYS_LOG_FILE_HANDLE = sys_file_zero();
	}
}

// _slog_itoa
static inline str8
sys_log_itoa(u32 x, str8 *buf)
{
	const usize max_digits = 10;
	if(buf->size < max_digits) { return str8_zero(); }

	u8 *cursor = buf->str + max_digits;
	do {
		cursor -= 1;
		*cursor = (u8)('0' + (x % 10));
		x /= 10;
	} while(x != 0);
	return str8_range(cursor, buf->str + max_digits);
}

void
sys_log(const char *tag, u32 log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename, void *userdata)
{
	(void)userdata;
	if(log_level > SYS_LOG_LEVEL) { return; }

	str8 tag8  = tag ? str8_cstr((char *)tag) : str8_zero();
	str8 msg8  = msg ? str8_cstr((char *)msg) : str8_zero();
	str8 file8 = filename ? str8_cstr((char *)filename) : str8_zero();

	str8 level8;
	switch(log_level) {
	case 0: level8 = str8_lit("panic"); break;
	case 1: level8 = str8_lit("error"); break;
	case 2: level8 = str8_lit("warning"); break;
	default: level8 = str8_lit("info"); break;
	}

	u8 line_mem[SYS_LOG_LINE_LENGTH];
	str8 line = {.str = line_mem, .size = 0};

	u8 num_mem[32];
	str8 num = str8_range(num_mem, num_mem + sizeof(num_mem));

	if(tag) {
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("["));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, tag8);
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("]"));
	}

	str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("["));
	str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, level8);
	str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("]"));

	if(log_item) {
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("[id:"));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, sys_log_itoa(log_item, &num));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("]"));
	}

	// if a filename is provided, build a clickable log message that's compatible with compiler error messages
	if(filename) {
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit(" "));
#if defined(_MSC_VER)
		// MSVC compiler error format
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, file8);
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("("));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, sys_log_itoa(line_nr, &num));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("): "));
#else
		// gcc/clang compiler error format
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, file8);
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit(":"));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, sys_log_itoa(line_nr, &num));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit(":0: "));
#endif
	} else if(line_nr) {
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("[line:"));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, sys_log_itoa(line_nr, &num));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("] "));
	}

	if(msg) {
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("\n\t"));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, msg8);
	}
	if(0 == log_level) {
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("\n"));
		str8_cat_in_place_bounded(&line, SYS_LOG_LINE_LENGTH, str8_lit("ABORTING because of [panic]\n"));
	}

#if SYS_LOG_FILE
	if(sys_file_is_valid(SYS_LOG_FILE_HANDLE)) {
		sys_file_w(SYS_LOG_FILE_HANDLE, line.str, (u32)line.size);
		sys_file_flush(SYS_LOG_FILE_HANDLE);
	}
#endif

#if SYS_LOG_STDIO
	sys_log_to_console(log_level, line);
#endif

	if(0 == log_level) {
		sys_abort();
	}
}
