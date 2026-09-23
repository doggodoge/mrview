#pragma once

#include <stdbool.h>
#include <adwaita.h>

#include "base.h"
#include "string_view.h"

typedef struct {
	i32         number;
	String_View title;
	String_View author;
	String_View url;
	bool        is_draft;
} Pull_Request_Card_Data;

GtkWidget *pull_request_card_new(Pull_Request_Card_Data data);
