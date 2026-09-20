#include "static_arena.h"

#include <string.h>

#include "base.h"

Static_Arena static_arena_init(u8 *buf_ptr, usize size) {
	return (Static_Arena){
		.buf_ptr = buf_ptr,
		.cap     = size,
	};
}

void static_arena_alloc(Static_Arena *static_arena, usize size) {
	if (static_arena->len + size > static_arena->cap) {
		fputs("Static_Arena over capacity.", stderr);
		abort();
	}

	void *return_address = (void *)(static_arena->buf_ptr[static_arena->len]);
	static_arena->len += size;

	return return_address;
}

void static_arena_reset(Static_Arena *static_arena) {
	memset(static_arena->buf_ptr, 0, static_arena->len);
	static_arena->len = 0;
}
