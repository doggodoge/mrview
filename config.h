#pragma once

#include "base.h"
#include "string_view.h"

#define MAX_LINES (256)

typedef struct {
	String_View  lines[MAX_LINES];
	usize        len;
	String_View _contents;
} Config_State;

void config_init(Config_State *state, String_View config_path);
void config_free(Config_State *state);
