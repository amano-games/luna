#pragma once

#include "base/mathfunc.h"
#include "base/str.h"
#include "base/types.h"
#include "sys/sys-io.h"
#include "sys/sys-log.h"
#include "sys/sys-printf.h"

static inline i32
csv_row_append(char *row, i32 cap, i32 used, const char *fmt, ...)
{
	va_list args;
	int w;
	i32 left;

	used = clamp_i32(used, 0, cap - 1);
	left = cap - used;
	va_start(args, fmt);
	w = sys_vsnprintf(row + used, left, fmt, args);
	va_end(args);

	if(w < 0) {
		return used;
	}
	if(w >= left) {
		return cap - 1;
	}

	return used + w;
}

static inline void
csv_row_push(struct alloc alloc, struct str8_list *list, char *row, i32 n, i32 cap)
{
	n      = clamp_i32(n, 0, cap - 1);
	row[n] = 0;
	str8_list_pushf(alloc, list, "%s\n", row);
}

static inline str8
csv_quote_push(struct alloc alloc, str8 s)
{
	ssize quotes = 0;
	for(usize i = 0; i < s.size; ++i) {
		if(s.str[i] == '"') { ++quotes; }
	}
	str8 res = {0};
	res.str  = alloc.allocf(alloc.ctx, (ssize)s.size + quotes + 2, alignof(u8));
	if(!res.str) { return res; }
	res.str[res.size++] = '"';
	for(usize i = 0; i < s.size; ++i) {
		if(s.str[i] == '"') { res.str[res.size++] = '"'; }
		res.str[res.size++] = s.str[i];
	}
	res.str[res.size++] = '"';
	return res;
}

static inline str8
csv_perc_push(struct alloc alloc, ssize value, ssize total)
{
	i32 permille = total > 0 ? (i32)((value * 1000) / total) : 0;
	return str8_fmt_push(alloc, "%d.%d", permille / 10, permille % 10);
}

// path: cache-dir/<app>-<timestamp>-<kind>.csv
static inline str8
csv_save(struct alloc alloc, str8 csv, str8 kind, str8 app_name, str8 app_org)
{
	str8 path = str8_fmt_push(
		alloc,
		"%.*s-%s-%.*s.csv",
		str8_spread(app_name),
		sys_path_timestamp(alloc).str,
		str8_spread(kind));

	str8 dir = sys_path_to_cache_path(alloc, str8_lit(""), app_org, app_name);
	if(dir.size > 0) {
		sys_make_dir(dir);
	}

	str8 full_path = sys_path_to_cache_path(alloc, path, app_org, app_name);
	sys_file f     = sys_file_open_w(full_path);

	if(!sys_file_is_valid(f)) {
		log_error("csv", "%.*s csv save failed: %.*s", str8_spread(kind), str8_spread(full_path));
		return (str8){0};
	}

	if(csv.size > 0) {
		sys_file_w(f, csv.str, (u32)csv.size);
	}

	sys_file_close(f);

	return full_path;
}
