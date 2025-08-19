#include "stack.h"
#include "sys-log.h"
#include "dbg.h"

void
stack_u8_init(struct stack_u8 *stack)
{
	mclr(stack, sizeof(struct stack_u8));
}

b32
stack_u8_push(struct stack_u8 *stack, u8 value)
{
	dbg_assert(!stack_u8_is_full(stack));
	stack->data[stack->count++] = value;
	return true;
}

u8
stack_u8_pop(struct stack_u8 *stack)
{
	if(stack_u8_is_empty(stack)) {
		return 0;
	}
	u8 res                    = stack->data[--stack->count];
	stack->data[stack->count] = 0;
	return res;
}

u8
stack_u8_peek(struct stack_u8 *stack)
{
	if(stack_u8_is_empty(stack)) {
		return 0;
	}
	return stack->data[stack->count - 1];
}

b32
stack_u8_is_full(struct stack_u8 *stack)
{
	return stack->count == STACK_MAX_ITEMS;
}

b32
stack_u8_is_empty(struct stack_u8 *stack)
{
	return stack->count == 0;
}

void
stack_u8_print(struct stack_u8 *stack)
{
	if(stack_u8_is_empty(stack)) {
		sys_printf("Stack is empty");
		return;
	}
	sys_printf("Stack contents:");
	for(int i = 0; i < stack->count; i++) {
		sys_printf("%d ", stack->data[i]);
	}
}
