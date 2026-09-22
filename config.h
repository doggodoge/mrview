#pragma once

#include <stdbool.h>

#include "base.h"
#include "string_view.h"

#define MAX_REPOSITORIES (256)
#define MAX_CONFIG_BYTES (64 * 1024)

typedef struct {
	String_View owner;
	String_View repo;
} Config_Repository;

typedef struct {
	Config_Repository repositories[MAX_REPOSITORIES];
	usize             len;
	char              contents[MAX_CONFIG_BYTES + 1];
} Config_State;

bool config_init(Config_State *state, char const *config_path);
