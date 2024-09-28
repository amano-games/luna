#pragma once

#include "sys-types.h"

struct exe_path {
	str8 path;
	str8 dirname;
};

struct exe_path sys_sokol_where(void);
