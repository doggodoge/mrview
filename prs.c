#include "prs.h"

#include <stdio.h>
#include <string.h>

#include "vendor/yyjson/yyjson.h"
#include "string_view.h"

#define REPOSITORY_MAX_LEN 255
#define COMMAND_CAPACITY 384

static void *json_arena_alloc       (void *context, size_t size);
static void *json_arena_realloc     (void *context, void *memory, size_t old_size, size_t new_size);
static void  json_arena_free        (void *context, void *memory);
static bool  is_repository_character(char c);
static bool  is_repository_valid    (String_View repository);
static bool  is_pr_valid            (const yyjson_val *title, const yyjson_val *url, const yyjson_val *number);

bool pull_requests_get(Static_Arena *arena, Pull_Requests *requests, String_View repository) {
	if (requests == NULL) {
		fprintf(stderr, "Pull request storage is not initialized\n");
		return false;
	}

	if (!is_repository_valid(repository)) {
		fprintf(stderr, "Repository must be in owner/repo form\n");
		return false;
	}

	char command[COMMAND_CAPACITY];
	int command_len = snprintf(command, sizeof command,
		"gh pr list --repo %.*s --limit %d --json number,title,author,url",
		(int)repository.len, repository.str, PULL_REQUEST_MAX_ITEMS);

	if (command_len < 0 || (usize)command_len >= sizeof command) {
		fprintf(stderr, "Could not construct gh command\n");
		return false;
	}

	FILE *f = popen(command, "r");
	if (f == NULL) {
		perror("Could not run gh");
		return false;
	}

	yyjson_alc json_allocator = {
		.malloc = json_arena_alloc,
		.realloc = json_arena_realloc,
		.free = json_arena_free,
		.ctx = arena,
	};
	yyjson_read_err error;
	yyjson_doc *doc = yyjson_read_fp(f, YYJSON_READ_NOFLAG, &json_allocator, &error);
	int command_status = pclose(f);

	if (command_status != 0) {
		fprintf(stderr, "gh pr list failed for %.*s\n",
			(int)repository.len, repository.str);
		return false;
	}

	if (!doc) {
		fprintf(stderr, "JSON error at byte: %zu: %s\n", error.pos, error.msg);
		return false;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);

	if (!yyjson_is_arr(root)) {
		fprintf(stderr, "Expected a JSON array\n");
		return false;
	}

	Pull_Requests loaded = {0};

	usize index;
	usize count;
	yyjson_val *pr;

	yyjson_arr_foreach(root, index, count, pr) {
		if (loaded.len == PULL_REQUEST_MAX_ITEMS) {
			break;
		}

		yyjson_val *title  = yyjson_obj_get(pr, "title");
		// yyjson_val *body   = yyjson_obj_get(pr, "body");
		yyjson_val *url    = yyjson_obj_get(pr, "url");
		yyjson_val *number = yyjson_obj_get(pr, "number");

		// "author" is itself an object
		yyjson_val *author = yyjson_obj_get(pr, "author");
		yyjson_val *login = yyjson_is_obj(author) ? yyjson_obj_get(author, "login") : NULL;

		if (!is_pr_valid(title, url, number)) {
			continue;
		}

		usize out = loaded.len++;

		loaded.title[out] = (String_View){
			.str = (char *)yyjson_get_str(title),
			.len = yyjson_get_len(title),
		};

		// loaded.body[out] = (String_View){
		// 	.str = (char *)yyjson_get_str(body),
		// 	.len = yyjson_get_len(body),
		// };

		loaded.url[out] = (String_View){
			.str = (char *)yyjson_get_str(url),
			.len = yyjson_get_len(url),
		};

		if (yyjson_is_str(login)) {
			loaded.author[out] = (String_View){
				.str = (char *)yyjson_get_str(login),
				.len = yyjson_get_len(login),
			};
		}

		loaded.number[out] = yyjson_get_int(number);
	}

	*requests = loaded;
	return true;
}

static void *json_arena_alloc(void *context, size_t size) {
	return static_arena_alloc(context, size);
}

static void *json_arena_realloc(void *context, void *memory, size_t old_size, size_t new_size) {
	return static_arena_realloc(context, memory, old_size, new_size);
}

static void json_arena_free(void *context, void *memory) {
	(void)context;
	(void)memory;
}

static bool is_repository_valid(String_View repository) {
	if (repository.str == NULL || repository.len < 3 ||
		repository.len > REPOSITORY_MAX_LEN) {
		return false;
	}

	usize slash_count = 0;
	for (usize i = 0; i < repository.len; i += 1) {
		char c = repository.str[i];
		if (c == '/') {
			if (i == 0 || i == repository.len - 1) {
				return false;
			}
			slash_count += 1;
		} else if (!is_repository_character(c)) {
			return false;
		}
	}

	return slash_count == 1;
}

static bool is_repository_character(char c) {
	return (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9')
		|| c == '-'
		|| c == '_'
		|| c == '.';
}

static bool is_pr_valid(const yyjson_val *title, const yyjson_val *url, const yyjson_val *number) {
	return yyjson_is_str(title)
		// && yyjson_is_str(body)
		&& yyjson_is_str(url)
		&& yyjson_is_int(number);
}
