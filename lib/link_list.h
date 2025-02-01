#pragma once

// NOTE: linked list macro helpers
// TODO: undestand it
#define CheckNil(nil, p)                    ((p) == 0 || (p) == nil)
#define SetNil(nil, p)                      ((p) = nil)
#define SLLQueuePush_NZ(nil, f, l, n, next) (CheckNil(nil, f) ? ((f) = (l) = (n), SetNil(nil, (n)->next)) : ((l)->next = (n), (l) = (n), SetNil(nil, (n)->next)))
#define SLLQueuePush(f, l, n)               SLLQueuePush_NZ(0, f, l, n, next)

// singly-linked, singly-headed lists (stacks)
#define SLLStackPush_N(f, n, next) ((n)->next = (f), (f) = (n))
#define SLLStackPop_N(f, next)     ((f) = (f)->next)
// singly-linked, singly-headed list helpers
#define SLLStackPush(f, n) SLLStackPush_N(f, n, next)
#define SLLStackPop(f)     SLLStackPop_N(f, next)
