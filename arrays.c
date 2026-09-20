#include "arrays.h"

#include <string.h>
#include <stdlib.h>

#include "arena.h"
#include "slices.h"

#define DEFAULT_CAP     (64)
#define GROWTH_CONSTANT (2)

void _dynamic_array_grow_if_required(Arena *arena, Dynamic_Array *array) {
	if (array->len == array->cap) {
		void *new_mem = arena_alloc(arena, array->cap * GROWTH_CONSTANT);
		memcpy(new_mem, array->mem, (array->elem_size * array->len));
		array->mem = new_mem;
	}
}

Dynamic_Array dynamic_array_init(Arena *arena, usize elem_size) {
	void *new_mem = arena_alloc(arena, elem_size * DEFAULT_CAP);

	return (Dynamic_Array){
		.mem = new_mem,
		.len = 0,
		.cap = DEFAULT_CAP,
		.elem_size = elem_size,		
	};
}

Dynamic_Array dynamic_array_init_with_capacity(Arena *arena, usize elem_size, usize capacity) {
	void *new_mem = arena_alloc(arena, elem_size * capacity);

	return (Dynamic_Array){
		.mem = new_mem,
		.len = 0,
		.cap = capacity,
		.elem_size = elem_size,		
	};
}

void dynamic_array_add(Dynamic_Array *array, Arena *arena, void *element) {
	_dynamic_array_grow_if_required(arena, array);

	usize scaled_index = array->len * array->elem_size;
	memcpy((u8 *)array->mem + scaled_index, element, array->elem_size);
	array->len += 1;
}

void dynamic_array_add_slice(Dynamic_Array *array, Arena *arena, Slice slice) {
	_dynamic_array_grow_if_required(arena, array);

	usize scaled_index = array->len * array->elem_size;
	usize slice_size   = slice_get_size(slice);

	memcpy((u8 *)array->mem + scaled_index, slice.mem, slice_size);
	array->len += slice_size;
}

void *dynamic_array_get(Dynamic_Array *array, usize index) {
	usize scaled_index = index * array->elem_size;
	return (u8 *)array->mem + scaled_index;
}
