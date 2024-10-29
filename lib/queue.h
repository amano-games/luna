#pragma once

#include "sys-types.h"

#define QUEUE_MAX_ITEMS 10

struct queue_u8 {
	u8 data[QUEUE_MAX_ITEMS];
	u8 head;
	u8 tail;
	u8 count;
};

void queue_u8_init(struct queue_u8 *queue);
bool32 enqueue_u8(struct queue_u8 *queue, u8 value);
u8 dequeue_u8(struct queue_u8 *queue);
u8 queue_u8_peek(struct queue_u8 *queue);
bool32 queue_u8_is_empty(struct queue_u8 *queue);
bool32 queue_u8_is_full(struct queue_u8 *queue);
void queue_u8_print(struct queue_u8 *queue);
