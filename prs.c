#include "prs.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "base.h"
#include "string_view.h"
#include "vendor/yyjson/yyjson.h"

#define REPOSITORY_MAX_LEN 255
#define COMMAND_CAPACITY 384

internal void *json_arena_alloc       (void *context, size_t size);
internal void *json_arena_realloc     (void *context, void *memory, size_t old_size, size_t new_size);
internal void  json_arena_free        (void *context, void *memory);
internal bool  is_repository_character(char c);
internal bool  is_repository_valid    (String_View repository);
internal bool  is_pr_valid            (yyjson_val const *title, yyjson_val const *url, yyjson_val const *number, yyjson_val const *is_draft);

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
	i32 command_len = snprintf(command, sizeof command,
		"gh pr list --repo %.*s --limit %d --json number,title,author,url,isDraft",
		(i32)repository.len, repository.str, PULL_REQUEST_MAX_ITEMS);

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
	i32 command_status = pclose(f);

	if (command_status != 0) {
		fprintf(stderr, "gh pr list failed for %.*s\n",
			(i32)repository.len, repository.str);
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

		yyjson_val *title    = yyjson_obj_get(pr, "title");
		yyjson_val *url      = yyjson_obj_get(pr, "url");
		yyjson_val *number   = yyjson_obj_get(pr, "number");
		yyjson_val *author   = yyjson_obj_get(pr, "author");
		yyjson_val *is_draft = yyjson_obj_get(pr, "isDraft");
		yyjson_val *login    = yyjson_is_obj(author) ? yyjson_obj_get(author, "login") : NULL;

		if (!is_pr_valid(title, url, number, is_draft)) {
			continue;
		}

		usize out = loaded.len++;

		loaded.title[out] = (String_View){
			.str = (char *)yyjson_get_str(title),
			.len = yyjson_get_len(title),
		};

		loaded.url[out] = (String_View){
			.str = (char *)yyjson_get_str(url),
			.len = yyjson_get_len(url),
		};

		loaded.is_draft[out] = yyjson_get_bool(is_draft);

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

Pull_Request pull_request_at_index(Pull_Requests *prs, usize index) {
	Pull_Request pr = {0};
	if (index > prs->len - 1) {
		return pr;
	}

	pr.title    = prs->title[index];
	pr.author   = prs->author[index];
	pr.url      = prs->url[index];
	pr.number   = prs->number[index];
	pr.is_draft = prs->is_draft[index];

	return pr;
}

internal void *json_arena_alloc(void *context, size_t size) {
	return static_arena_alloc(context, size);
}

internal void *json_arena_realloc(void *context, void *memory, size_t old_size, size_t new_size) {
	return static_arena_realloc(context, memory, old_size, new_size);
}

internal void json_arena_free(void *context, void *memory) {
	(void)context;
	(void)memory;
}

internal bool is_repository_valid(String_View repository) {
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

internal bool is_repository_character(char c) {
	return (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9')
		|| c == '-'
		|| c == '_'
		|| c == '.';
}

internal bool is_pr_valid(yyjson_val const *title, yyjson_val const *url, yyjson_val const *number, yyjson_val const *is_draft) {
	return yyjson_is_str(title)
		&& yyjson_is_str(url)
		&& yyjson_is_int(number)
		&& yyjson_is_bool(is_draft);
}
