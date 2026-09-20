#include "prs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "string_view.h"
#include "arrays.h"
#include "slices.h"

void pull_requests_get_for_url(Arena *arena, Pull_Requests *requests, String_View url) {
	FILE *f = popen("gh pr view --json \"id,title,author,body,curl\"", "r");
	Dynamic_Array char_arr = dynamic_array_init(arena, sizeof(char));

	char buf[4096];
	while (fgets(buf, sizeof buf, f)) {
		Slice slice = (Slice){
			.mem       = &buf,
			.len       = strlen(buf),
			.elem_size = sizeof(char),
		};

		dynamic_array_add_slice(&char_arr, arena, slice);
	}

	printf("%s", (char *)char_arr.mem);

	pclose(f);
}
