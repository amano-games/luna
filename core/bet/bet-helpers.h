#pragma once

#include "sys-types.h"

#define MAX_BET_STACK 100
#define MAX_BET_QUEUE 100

struct queue_u8 {
	u8 data[MAX_BET_STACK];
	u8 head;
	u8 tail;
	u8 count;
};

struct stack_u8 {
	u8 data[MAX_BET_QUEUE];
	u8 count;
};

void stack_u8_init(struct stack_u8 *stack);
bool32 stack_u8_push(struct stack_u8 *stack, u8 value);
u8 stack_u8_pop(struct stack_u8 *stack);
u8 stack_u8_pop_back(struct stack_u8 *stack);
u8 stack_u8_peek(struct stack_u8 *stack);
bool32 stack_u8_is_full(struct stack_u8 *stack);
bool32 stack_u8_is_empty(struct stack_u8 *stack);
void stack_u8_print(struct stack_u8 *stack);

void enqueue_u8_init(struct queue_u8 *queue);
bool32 enqueue_u8(struct queue_u8 *queue, u8 value);
u8 dequeue_u8(struct queue_u8 *queue);
u8 queue_u8_peek(struct queue_u8 *queue);
bool32 queue_u8_is_empty(struct queue_u8 *queue);
bool32 queue_u8_is_full(struct queue_u8 *queue);
void queue_u8_print(struct queue_u8 *queue);
