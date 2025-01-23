#pragma once

#include "bet/bet.h"

void bet_v2_ctx_init(struct bet_ctx *ctx);
enum bet_res bet_v2_tick(struct bet *bet, struct bet_ctx *ctx, void *userdata);
