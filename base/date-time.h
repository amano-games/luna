#pragma once

#include "base/types.h"

#define SECONDS_IN_A_DAY 86400

enum week_day {
	WEEK_DAY_SUN,
	WEEK_DAY_MON,
	WEEK_DAY_TUE,
	WEEK_DAY_WED,
	WEEK_DAY_THU,
	WEEK_DAY_FRI,
	WEEK_DAY_SAT,
	WEEK_DAY_NUM_COUNT,
};

enum month {
	MONTH_JAN,
	MONTH_FEB,
	MONTH_MAR,
	MONTH_APR,
	MONTH_MAY,
	MONTH_JUN,
	MONTH_JUL,
	MONTH_AUG,
	MONTH_SEP,
	MONTH_OCT,
	MONTH_NOV,
	MONTH_DEC,
	MONTH_NUM_COUNT,
};

typedef u64 dense_time;
struct date_time {
	u16 micro_sec; // [0,999]
	u16 msec;      // [0,999]
	u16 sec;       // [0,60]
	u16 min;       // [0,59]
	u16 hour;      // [0,24]
	u16 day;       // [0,30]
	union {
		enum week_day week_day;
		u32 wday;
	};
	union {
		enum month month;
		u32 mon;
	};
	u32 year; // 1 = 1 CE, 0 = 1 BC
};

dense_time dense_time_from_date_time(struct date_time date_time);

struct date_time date_time_from_dense_time(dense_time time);
struct date_time date_time_from_micro_seconds(u64 time);
struct date_time date_time_from_unix_time_gmt(u64 unix_time);
struct date_time date_time_from_epoch_2000_gmt(u64 seconds);
