#include "queue.h"
#include "sys-log.h"
#include "dbg.h"
#include "sys-utils.h"

void
queue_u8_init(struct queue_u8 *queue)
{
	if(!queue_u8_is_empty(queue)) {
		for(int i = 0; i < queue->count; i++) {
			int index          = (queue->head + i) % QUEUE_MAX_ITEMS;
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
	return queue->count == QUEUE_MAX_ITEMS;
}

bool32
queue_u8_is_empty(struct queue_u8 *queue)
{
	return queue->count == 0;
}

bool32
enqueue_u8(struct queue_u8 *queue, u8 value)
{
	// assert(!queue_u8_is_full(queue));

	queue->data[queue->tail] = value;                               // Add the value at the tail
	queue->tail              = (queue->tail + 1) % QUEUE_MAX_ITEMS; // Circular increment of tail
	queue->count             = MIN(queue->count + 1, QUEUE_MAX_ITEMS);
	// bet_queue_print(queue);
	return true;
}

u8
dequeue_u8(struct queue_u8 *queue)
{
	if(queue_u8_is_empty(queue)) {
		return 0;
	}

	u8 value                 = queue->data[queue->head];            // Get the value at the head
	queue->data[queue->head] = 0;                                   // clear the value
	queue->head              = (queue->head + 1) % QUEUE_MAX_ITEMS; // Circular increment of head
	queue->count--;                                                 // Decrease the size of the queue
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
		sys_printf("Queue empty");
		return;
	}

	sys_printf("Queue contents: ");
	for(int i = 0; i < queue->count; i++) {
		int index = (queue->head + i) % QUEUE_MAX_ITEMS;
		sys_printf("%d ", queue->data[index]);
	}
}
