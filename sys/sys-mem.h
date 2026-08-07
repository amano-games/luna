#pragma once

#include "base/types.h"

void *sys_alloc_aligned_raw(ssize size, ssize align, void *(*raw_alloc)(ssize bytes));
void sys_free_aligned_raw(void *p, void (*raw_free)(void *));
