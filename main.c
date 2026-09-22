#include <adwaita.h>
#include <stdio.h>
#include <stdlib.h>

#include "base.h"
#include "prs.h"
#include "pull_request_card.h"
#include "static_arena.h"
#include "string_view.h"
#include "config.h"

#define PR_STORAGE_CAPACITY (16 * 1024 * 1024)
#define TEST_REPOSITORY "ghostty-org/ghostty"

typedef struct {
	Static_Arena pr_arena;
	Pull_Requests pull_requests;
	bool pull_requests_loaded;
} App_State;

global_variable App_State app_state;
global_variable _Alignas(max_align_t) u8 pr_storage[PR_STORAGE_CAPACITY];

global_variable Config_State config;
global_variable char config_path[4096];

internal GtkWidget *pull_request_list_new(const Pull_Requests *requests) {
	GtkWidget *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	for (usize i = 0; i < requests->len; i += 1) {
		Pull_Request_Card_Data data = {
			.number = requests->number[i],
			.title = requests->title[i],
			.description = requests->body[i],
			.author = requests->author[i],
			.url = requests->url[i],
		};

		gtk_box_append(GTK_BOX(list), pull_request_card_new(data));
	}

	return list;
}

internal void on_activate(GtkApplication *app, void *user_data) {
	App_State *state = user_data;

	GtkWidget *window = adw_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "MRView");
	gtk_window_set_default_size(GTK_WINDOW(window), 900, 720);

	GtkWidget *toolbar_view = adw_toolbar_view_new();
	GtkWidget *header_bar = adw_header_bar_new();
	GtkWidget *window_title = adw_window_title_new("MRView", TEST_REPOSITORY);
	adw_header_bar_set_title_widget(ADW_HEADER_BAR(header_bar), window_title);
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header_bar);

	GtkWidget *scrolled = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view), scrolled);

	GtkWidget *clamp = adw_clamp_new();
	adw_clamp_set_maximum_size(ADW_CLAMP(clamp), 760);
	adw_clamp_set_tightening_threshold(ADW_CLAMP(clamp), 600);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), clamp);

	GtkWidget *content;
	if (state->pull_requests_loaded && state->pull_requests.len > 0) {
		content = pull_request_list_new(&state->pull_requests);
	} else if (state->pull_requests_loaded) {
		content = gtk_label_new("No open pull requests.");
	} else {
		content = gtk_label_new("Could not load pull requests. Check gh authentication and try again.");
		gtk_label_set_wrap(GTK_LABEL(content), true);
	}

	gtk_widget_set_margin_top(content, 18);
	gtk_widget_set_margin_bottom(content, 18);
	gtk_widget_set_margin_start(content, 18);
	gtk_widget_set_margin_end(content, 18);
	adw_clamp_set_child(ADW_CLAMP(clamp), content);

	adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), toolbar_view);
	gtk_window_present(GTK_WINDOW(window));
}

i32 main(i32 argc, char *argv[]) {
	char const *home = getenv("HOME");
	int const path_len = snprintf(config_path, sizeof config_path,
		"%s/.config/mrview/config", home);
	if (path_len < 0 || (usize)path_len >= sizeof config_path) {
		fprintf(stderr, "Could not load config: path is too long\n");
	} else if (!config_init(&config, config_path)) {
		fprintf(stderr, "Could not load config: %s\n", config_path);
	}

	for (usize i = 0; i < config.len; i += 1) {
		Config_Repository repository = config.repositories[i];
		printf("%.*s/%.*s\n", (i32)repository.owner.len, repository.owner.str,
			(i32)repository.repo.len, repository.repo.str);
	}

	app_state.pr_arena = static_arena_init(pr_storage, sizeof pr_storage);
	app_state.pull_requests_loaded = pull_requests_get(
		&app_state.pr_arena,
		&app_state.pull_requests,
		string_view_from_cstr(TEST_REPOSITORY));

	g_autoptr(AdwApplication) app = adw_application_new("net.mooremoore.MRView", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(on_activate), &app_state);

	i32 status = g_application_run(G_APPLICATION(app), argc, argv);

	static_arena_reset(&app_state.pr_arena);
	return status;
}
