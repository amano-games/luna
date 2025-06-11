#pragma once

#include "arr.h"
#include "mem.h"
#include "sys-types.h"

static u32 *
bit_arr_u32_new(usize count, struct alloc alloc)
{
	usize bit_arr_count = (count + 31) / 32;
	u32 *res            = arr_ini(bit_arr_count, sizeof(*res), alloc);
	arr_clr(res);
	return res;
}

static bool32
bit_arr_u32_exists_and_set(u32 *arr, usize count, usize index)
{
	// Calculate the index of the integer in the array
	usize integer_index = index >> 5;
	// Lookup the corresponding bit in the appropiated word (32bits)
	u32 mask = UINT32_C(1) << (index & 31);

	if((arr[integer_index] & mask) == 0) {
		// Bit not set, so pair has not already been processed;
		// process object pair for intersection now
		// Mark object pair as processed
		arr[integer_index] |= mask;
		return false;
	}
	return true;
}

static u32 *
bit_pairs_arr_u32_new(usize count, struct alloc alloc)
{
	usize max_pairs_count = count * (count - 1) / 2;
	return bit_arr_u32_new(max_pairs_count, alloc);
}

static bool32
bit_pair_arr_exists_and_set(u32 *arr, usize count, usize index_a, usize index_b)
{
	assert(index_a != index_b);
	u32 min = index_a;
	u32 max = index_b;
	if(index_b < index_a) {
		min = index_b;
		max = index_a;
	}

	// Compute index of bit representing the object pair in the array
	u32 index = min * (2 * count - min - 3) / 2 + max - 1;
	return bit_arr_u32_exists_and_set(arr, count, index);
}
