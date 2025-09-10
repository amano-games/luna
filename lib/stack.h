#pragma once

#include "base/types.h"

#define STACK_MAX_ITEMS 100

struct stack_u8 {
	u8 data[STACK_MAX_ITEMS];
	u8 count;
};

void stack_u8_init(struct stack_u8 *stack);
b32 stack_u8_push(struct stack_u8 *stack, u8 value);
u8 stack_u8_pop(struct stack_u8 *stack);
u8 stack_u8_pop_back(struct stack_u8 *stack);
u8 stack_u8_peek(struct stack_u8 *stack);
b32 stack_u8_is_full(struct stack_u8 *stack);
b32 stack_u8_is_empty(struct stack_u8 *stack);
void stack_u8_print(struct stack_u8 *stack);
