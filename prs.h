#pragma once

#include <stdbool.h>

#include "base.h"
#include "static_arena.h"
#include "string_view.h"

#define PULL_REQUEST_MAX_ITEMS 2048

typedef struct {
	String_View title;
	String_View author;
	String_View url;
	i32         number;
	bool        is_draft;
} Pull_Request;

typedef struct {
	String_View  title     [PULL_REQUEST_MAX_ITEMS];
	String_View  author    [PULL_REQUEST_MAX_ITEMS];
	String_View  url       [PULL_REQUEST_MAX_ITEMS];
	i32          number    [PULL_REQUEST_MAX_ITEMS];
	bool         is_draft  [PULL_REQUEST_MAX_ITEMS];
	usize        len;
} Pull_Requests;

// `repository` must be in "owner/repo" form. The returned string views remain valid until the arena is reset.
bool         pull_requests_get(Static_Arena *arena, Pull_Requests *requests, String_View repository);
Pull_Request pull_request_at_index(Pull_Requests *prs, usize index);
