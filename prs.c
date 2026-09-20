#include "prs.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "vendor/yyjson/yyjson.h"
#include "string_view.h"

static bool is_repository_valid(String_View repository);
static bool is_pr_valid(const yyjson_val *title, const yyjson_val *body,
	const yyjson_val *url, const yyjson_val *number);

bool pull_requests_get(Pull_Requests *requests, String_View repository) {
	if (requests == NULL || !is_repository_valid(repository)) {
		fprintf(stderr, "Repository must be in owner/repo form\n");
		return false;
	}

	char *repository_cstr = g_strndup(repository.str, repository.len);
	char *quoted_repository = g_shell_quote(repository_cstr);
	char *command = g_strdup_printf(
		"gh pr list --repo %s --limit %d --json number,title,author,body,url",
		quoted_repository, PULL_REQUEST_MAX_ITEMS);

	FILE *f = popen(command, "r");
	g_free(command);
	g_free(quoted_repository);
	g_free(repository_cstr);

	if (f == NULL) {
		perror("Could not run gh");
		return false;
	}

	yyjson_read_err error;
	yyjson_doc *doc = yyjson_read_fp(f, YYJSON_READ_NOFLAG, NULL, &error);
	int command_status = pclose(f);

	if (command_status != 0) {
		fprintf(stderr, "gh pr list failed for %.*s\n",
			(int)repository.len, repository.str);
		yyjson_doc_free(doc);
		return false;
	}

	if (!doc) {
		fprintf(stderr, "JSON error at byte: %zu: %s\n", error.pos, error.msg);
		return false;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);

	if (!yyjson_is_arr(root)) {
		fprintf(stderr, "Expected a JSON array\n");
		yyjson_doc_free(doc);
		return false;
	}

	Pull_Requests loaded = {
		.document = doc,
	};

	usize index;
	usize count;
	yyjson_val *pr;

	yyjson_arr_foreach(root, index, count, pr) {
		if (loaded.len == PULL_REQUEST_MAX_ITEMS) {
			break;
		}

		yyjson_val *title  = yyjson_obj_get(pr, "title");
		yyjson_val *body   = yyjson_obj_get(pr, "body");
		yyjson_val *url    = yyjson_obj_get(pr, "url");
		yyjson_val *number = yyjson_obj_get(pr, "number");

		// "author" is itself an object
		yyjson_val *author = yyjson_obj_get(pr, "author");
		yyjson_val *login = yyjson_is_obj(author) ? yyjson_obj_get(author, "login") : NULL;

		if (!is_pr_valid(title, body, url, number)) {
			continue;
		}

		usize out = loaded.len++;

		loaded.title[out] = (String_View){
			.str = (char *)yyjson_get_str(title),
			.len = yyjson_get_len(title),
		};

		loaded.body[out] = (String_View){
			.str = (char *)yyjson_get_str(body),
			.len = yyjson_get_len(body),
		};

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

	pull_requests_clear(requests);
	*requests = loaded;
	return true;
}

void pull_requests_clear(Pull_Requests *requests) {
	if (requests == NULL) {
		return;
	}

	yyjson_doc_free(requests->document);
	*requests = (Pull_Requests){0};
}

static bool is_repository_valid(String_View repository) {
	if (repository.str == NULL || repository.len < 3 ||
		memchr(repository.str, '\0', repository.len) != NULL) {
		return false;
	}

	const char *slash = memchr(repository.str, '/', repository.len);
	return slash != NULL && slash != repository.str &&
		slash != repository.str + repository.len - 1 &&
		memchr(slash + 1, '/', (usize)(repository.str + repository.len - slash - 1)) == NULL;
}

static bool is_pr_valid(const yyjson_val *title, const yyjson_val *body,
	const yyjson_val *url, const yyjson_val *number) {
	return yyjson_is_str(title)
		&& yyjson_is_str(body)
		&& yyjson_is_str(url)
		&& yyjson_is_int(number);
}
