#include "slices.h"

#include "base.h"

usize slice_get_size(Slice slice) {
	return slice.len * slice.elem_size;
}
