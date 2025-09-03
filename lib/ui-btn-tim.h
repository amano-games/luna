#pragma once

#include "input.h"
#include "sys-types.h"

#define UI_BTN_TIM_M_COUNT 5
#define UI_BTN_TIM_L_COUNT 10
#define UI_BTN_TIM_RATE_S  0.2f
#define UI_BTN_TIM_RATE_M  0.1f
#define UI_BTN_TIM_RATE_L  0.05f

struct ui_btn_tim {
	usize count;
	f32 timestamp;
	u8 btn;
};

static inline f32
ui_btn_tim_get_rate(struct ui_btn_tim *tmr)
{
	f32 res = UI_BTN_TIM_RATE_S;

	if(tmr->count > UI_BTN_TIM_L_COUNT) {
		res = UI_BTN_TIM_RATE_L;
	} else if(tmr->count > UI_BTN_TIM_M_COUNT) {
		res = UI_BTN_TIM_RATE_M;
	}

	return res;
}

b32
ui_btn_tim_upd(struct ui_btn_tim *tim, f32 timestamp)
{
	b32 res = false;
	if(inp_just_pressed(tim->btn)) {
		tim->timestamp = timestamp;
		res            = true;
		goto cleanup;
	}

	if(inp_pressed(tim->btn)) {
		f32 rate = ui_btn_tim_get_rate(tim);
		if(tim->timestamp + rate < timestamp) {
			res            = true;
			tim->timestamp = timestamp;
			tim->count++;
		}
	} else {
		tim->count = 0;
	}

cleanup:;
	return res;
}
