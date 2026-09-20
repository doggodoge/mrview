#pragma once

#include "base.h"
#include "string_view.h"

#define PULL_REQUEST_MAX_ITEMS 2048

typedef struct yyjson_doc yyjson_doc;

typedef struct {
	String_View title[PULL_REQUEST_MAX_ITEMS];
	String_View body[PULL_REQUEST_MAX_ITEMS];
	String_View author[PULL_REQUEST_MAX_ITEMS];
	String_View url[PULL_REQUEST_MAX_ITEMS];
	i32 number[PULL_REQUEST_MAX_ITEMS];
	usize len;
	yyjson_doc *document;
} Pull_Requests;

// `repository` must be in "owner/repo" form. The returned string views remain
// valid until the requests are cleared or loaded again.
bool pull_requests_get(Pull_Requests *requests, String_View repository);
void pull_requests_clear(Pull_Requests *requests);
