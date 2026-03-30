#pragma once

#include "base/types.h"

extern int (*PD_SCORE_ADD)(const char *board_id, uint32_t value, AddScoreCallback callback);
extern void (*PD_SCORE_FREE)(PDScore *score);
extern int (*PD_SCORES_GET)(const char *board_id, ScoresCallback callback);
extern void (*PD_SCORES_LIST_FREE)(PDScoresList *scores_list);
extern void (*PD_SCORE_FREE)(PDScore *score);
extern int (*PD_PERSONAL_BEST_GET)(const char *board_id, PersonalBestCallback callback);
