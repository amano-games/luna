#include "date-time.h"
#include "dbg.h"

#define SECONDS_IN_A_DAY 86400

dense_time
dense_time_from_date_time(struct date_time date_time)
{
	dense_time res = 0;

	res += date_time.year;
	res *= 12;
	res += date_time.mon;
	res *= 31;
	res += date_time.day;
	res *= 24;
	res += date_time.hour;
	res *= 60;
	res += date_time.min;
	res *= 61;
	res += date_time.sec;
	res *= 1000;
	res += date_time.msec;

	return (res);
}

struct date_time
date_time_from_dense_time(dense_time time)
{
	struct date_time res = {0};
	res.msec             = time % 1000;
	time /= 1000;
	res.sec = time % 61;
	time /= 61;
	res.min = time % 60;
	time /= 60;
	res.hour = time % 24;
	time /= 24;
	res.day = time % 31;
	time /= 31;
	res.mon = time % 12;
	time /= 12;
	dbg_assert(time <= U32_MAX);
	res.year = (u32)time;

	return (res);
}

struct date_time
date_time_from_micro_seconds(u64 time)
{
	struct date_time res = {0};
	res.micro_sec        = time % 1000;
	time /= 1000;
	res.msec = time % 1000;
	time /= 1000;
	res.sec = time % 60;
	time /= 60;
	res.min = time % 60;
	time /= 60;
	res.hour = time % 24;
	time /= 24;
	res.day = time % 31;
	time /= 31;
	res.mon = time % 12;
	time /= 12;
	dbg_assert(time <= U32_MAX);
	res.year = (u32)time;

	return (res);
}

struct epoch_base {
	u32 year;
	enum week_day week_day_0;
};

static inline struct date_time
date_time_from_epoch_gmt(u64 seconds, struct epoch_base base)
{
	struct date_time date = {0};
	u64 days              = seconds / SECONDS_IN_A_DAY;
	date.year             = base.year;
	date.day              = 1 + days;
	date.sec              = (u32)seconds % 60;
	date.min              = (u32)(seconds / 60) % 60;
	date.hour             = (u32)(seconds / 3600) % 24;
	date.week_day         = (base.week_day_0 + days) % 7;

	for(;;) {
		for(date.month = 0; date.month < 12; ++date.month) {
			u64 c = 0;
			switch(date.month) {
			case MONTH_JAN: c = 31; break;
			case MONTH_FEB: {
				if((date.year % 4 == 0) && ((date.year % 100) != 0 || (date.year % 400) == 0)) {
					c = 29;
				} else {
					c = 28;
				}
			} break;
			case MONTH_MAR: c = 31; break;
			case MONTH_APR: c = 30; break;
			case MONTH_MAY: c = 31; break;
			case MONTH_JUN: c = 30; break;
			case MONTH_JUL: c = 31; break;
			case MONTH_AUG: c = 31; break;
			case MONTH_SEP: c = 30; break;
			case MONTH_OCT: c = 31; break;
			case MONTH_NOV: c = 30; break;
			case MONTH_DEC: c = 31; break;
			default: dbg_sentinel("date-time");
			}
			if(date.day <= c) {
				goto exit;
			}
			date.day -= c;
		}
		++date.year;
	}

exit:;
error:;

	return date;
}

static const struct epoch_base EPOCH_1970 = {1970, WEEK_DAY_THU};
static const struct epoch_base EPOCH_2000 = {2000, WEEK_DAY_SAT};

struct date_time
date_time_from_unix_time_gmt(u64 unix_time)
{
	struct date_time res = date_time_from_epoch_gmt(unix_time, EPOCH_1970);
	return res;
}

struct date_time
date_time_from_epoch_2000_gmt(u64 seconds)
{
	struct date_time res = date_time_from_epoch_gmt(seconds, EPOCH_2000);
	return res;
}
