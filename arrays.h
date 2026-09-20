#pragma once

#include "string_view.h"
#include "base.h"
#include "slices.h"

typedef struct {
	void *mem;
	usize len;
	usize cap;
	usize elem_size;
} Dynamic_Array;

Dynamic_Array dynamic_array_init(Arena *arena, usize elem_size);
Dynamic_Array dynamic_array_init_with_capacity(Arena *arena, usize elem_size, usize capacity);

void dynamic_array_add(Dynamic_Array *array, Arena *arena, void *element);
void dynamic_array_add_slice(Dynamic_Array *array, Arena *arena, Slice slice);

void *dynamic_array_get(Dynamic_Array *array, usize index);
