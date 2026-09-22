#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "string_view.h"

bool config_init(Config_State *config, char const *config_path) {
	*config = (Config_State){0};

	int const fd = open(config_path, O_RDONLY);
	if (fd < 0) {
		return false;
	}

	usize size = 0;
	while (size < sizeof config->contents) {
		ssize_t const n = read(fd, config->contents + size, sizeof config->contents - size);
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n < 0) {
			close(fd);
			return false;
		}
		if (n == 0) {
			break;
		}
		size += (usize)n;
	}
	close(fd);
	if (size > MAX_CONFIG_BYTES) {
		return false;
	}

	String_View contents = {.str = config->contents, .len = size};
	String_View_Iterator iter = string_view_split_lines_iterator_create(contents);
	while (iter.has_next) {
		String_View line = string_view_iterator_next(&iter);
		if (line.len == 0) {
			continue;
		}
		if (config->len == MAX_REPOSITORIES) {
			config->len = 0;
			return false;
		}
		String_View_Iterator parts = string_view_split_byte_iterator_create(line, '/');
		config->repositories[config->len] = (Config_Repository){
			.owner = string_view_iterator_next(&parts),
			.repo = string_view_iterator_next(&parts),
		};
		config->len += 1;
	}
	return true;
}
