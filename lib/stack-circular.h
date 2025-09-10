#pragma once

#include "base/types.h"
#include "base/utils.h"

#define STACK_CIR_MAX_ITEMS 100

struct stack_cir_u8 {
	u8 data[STACK_CIR_MAX_ITEMS];
	u8 capacity;
	u8 count;
	u8 head;
	u8 tail;
};

void
stack_cir_u8_init(struct stack_cir_u8 *stack)
{
	mclr(stack, sizeof(struct stack_cir_u8));
	stack->capacity = ARRLEN(stack->data);
}

b32
stack_cir_u8_is_empty(struct stack_cir_u8 *stack)
{
	return stack->count == 0;
}

b32
stack_cir_u8_is_full(struct stack_cir_u8 *stack)
{
	return stack->count == stack->capacity;
}

b32
stack_cir_u8_push(struct stack_cir_u8 *stack, u8 value)
{
	if(stack_cir_u8_is_full(stack)) {
		// If full, increment head to remove the oldest element
		stack->head = (stack->head + 1) % stack->capacity;
	} else {
		// Only increase count if we're not overwriting an existing element
		stack->count++;
	}

	// Place the new element at the tail
	stack->data[stack->tail] = value;
	// Move tail to the next position
	stack->tail = (stack->tail + 1) % stack->capacity;

	return true;
}

u8
stack_cir_u8_pop(struct stack_cir_u8 *stack)
{
	if(stack_cir_u8_is_empty(stack)) {
		return 0; // Nothing to pop
	}

	// Move tail back to the last element
	stack->tail = (stack->tail == 0) ? stack->capacity - 1 : stack->tail - 1;
	u8 res      = stack->data[stack->tail];
	stack->count--;

	return res;
}

u8
stack_cir_u8_peek(struct stack_cir_u8 *stack)
{
	if(stack_cir_u8_is_empty(stack)) {
		return 0; // Nothing to peek
	}

	// Peek at the last element (tail - 1)
	size_t last_index = (stack->tail == 0) ? stack->capacity - 1 : stack->tail - 1;
	u8 res            = stack->data[last_index];

	return res;
}
