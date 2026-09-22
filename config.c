#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "string_view.h"

void config_init(Config_State *config, String_View config_path) {
	*config = Config_State{0};

	String_View contents = read_entire_file(config_path.str);
	config->_contents = contents;

	String_View_Iterator iter = string_view_split_lines_iterator_create(contents);
	for (usize i = 0; iter.has_next; i += 1) {
		String_View line = string_view_iterator_next(&iter);
		config->lines[i] = line;
		config->len += 1;
	}
}

void config_free(Config_State *config) {
	memset(config->lines, 0, sizeof(String_View) * config->len);
	config->len = 0;
	free(config->_contents.str);
}
