#pragma once

#include "string_view.h"

typedef struct {
	i32         id,
	String_View title,
	String_View description,
	String_View author,
} Pull_Request_Card_State;

GtkWidget *pull_request_card_new(char *title, char *description);
