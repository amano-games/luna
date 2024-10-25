#include "bet-helpers.h"
#include "sys-log.h"
#include "sys-assert.h"

void
stack_u8_init(struct stack_u8 *stack)
{
	stack->count = 0;
	for(usize i = 0; i < MAX_BET_STACK; ++i) {
		stack->data[i] = 0;
	}
}

bool32
stack_u8_push(struct stack_u8 *stack, u8 value)
{
	assert(!stack_u8_is_full(stack));
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

bool32
stack_u8_is_full(struct stack_u8 *stack)
{
	return stack->count == MAX_BET_STACK;
}

bool32
stack_u8_is_empty(struct stack_u8 *stack)
{
	return stack->count == 0;
}

void
enqueue_u8_init(struct queue_u8 *queue)
{
	if(!queue_u8_is_empty(queue)) {
		for(int i = 0; i < queue->count; i++) {
			int index          = (queue->head + i) % MAX_BET_QUEUE;
			queue->data[index] = 0;
		}
	}
	queue->head  = 0;
	queue->tail  = 0;
	queue->count = 0;
}

bool32
queue_u8_is_full(struct queue_u8 *queue)
{
	return queue->count == MAX_BET_QUEUE;
}

bool32
queue_u8_is_empty(struct queue_u8 *queue)
{
	return queue->count == 0;
}

bool32
enqueue_u8(struct queue_u8 *queue, u8 value)
{
	assert(!queue_u8_is_full(queue));

	queue->data[queue->tail] = value;                             // Add the value at the tail
	queue->tail              = (queue->tail + 1) % MAX_BET_QUEUE; // Circular increment of tail
	queue->count++;                                               // Increase the size of the queue
	// bet_queue_print(queue);
	return true;
}

u8
dequeue_u8(struct queue_u8 *queue)
{
	if(queue_u8_is_empty(queue)) {
		return 0;
	}

	u8 value                 = queue->data[queue->head];          // Get the value at the head
	queue->data[queue->head] = 0;                                 // clear the value
	queue->head              = (queue->head + 1) % MAX_BET_QUEUE; // Circular increment of head
	queue->count--;                                               // Decrease the size of the queue
	// bet_queue_print(queue);
	return value;
}

u8
queue_u8_peek(struct queue_u8 *queue)
{
	if(queue_u8_is_empty(queue)) {
		return 0;
	}

	u8 value = queue->data[queue->head]; // Peek the value at the head
	return value;
}

// Print the queue for debugging purposes
void
queue_u8_print(struct queue_u8 *queue)
{
	if(queue_u8_is_empty(queue)) {
		sys_printf("Queue is empty");
		return;
	}

	sys_printf("Queue contents: ");
	for(int i = 0; i < queue->count; i++) {
		int index = (queue->head + i) % MAX_BET_QUEUE;
		sys_printf("%d ", queue->data[index]);
	}
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
