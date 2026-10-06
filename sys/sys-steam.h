#pragma once

#include "base/types.h"

b32 sys_steam_ini(void);
void sys_steam_tick(void);
void sys_steam_close(void);
b32 sys_steam_ok(void);

void sys_steam_achievement_unlock(str8 api_name);
void sys_steam_stat_set(str8 api_name, i32 value);
void sys_steam_stats_store(void);
