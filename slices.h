#pragma once

#include "base.h"

typedef struct {
	void *mem;
	usize len;
	u8    elem_size;
} Slice;

usize slice_get_size(Slice slice);
