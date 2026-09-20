#pragma once

#include "base.h"

typedef struct {
	u8         *buf;
	usize       len;
	usize const cap;
} Static_Arena;

Static_Arena static_arena_init(void *buf_ptr, usize size);
void static_arena_alloc(Static_Arena *static_arena, usize size);
void static_arena_reset(Static_Arena *static_arena);
