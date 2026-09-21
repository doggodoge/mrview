#pragma once

#include "base.h"
#include "static_arena.h"
#include "string_view.h"

#define PULL_REQUEST_MAX_ITEMS 2048

typedef struct {
	String_View title[PULL_REQUEST_MAX_ITEMS];
	String_View body[PULL_REQUEST_MAX_ITEMS];
	String_View author[PULL_REQUEST_MAX_ITEMS];
	String_View url[PULL_REQUEST_MAX_ITEMS];
	i32 number[PULL_REQUEST_MAX_ITEMS];
	usize len;
} Pull_Requests;

// `repository` must be in "owner/repo" form. The returned string views remain valid until the arena is reset.
bool pull_requests_get(Static_Arena *arena, Pull_Requests *requests, String_View repository);
