#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "base.h"

bool alignment_is_power_of_two(usize value);

// Alignment must be non-zero. Forward alignment can also fail on overflow.
bool align_forward(usize value, usize alignment, usize *result);
bool align_backward(usize value, usize alignment, usize *result);
bool is_aligned(usize value, usize alignment);

// Address variants avoid assuming that usize and uintptr_t have equal ranges.
bool align_forward_address(uintptr_t address, usize alignment, uintptr_t *result);
bool align_backward_address(uintptr_t address, usize alignment, uintptr_t *result);
bool is_address_aligned(uintptr_t address, usize alignment);
