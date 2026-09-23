#define _POSIX_C_SOURCE 200809L

#include <adwaita.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "base.h"
#include "prs.h"
#include "pull_request_card.h"
#include "static_arena.h"
#include "string_view.h"
#include "config.h"

#define PR_STORAGE_CAPACITY (16 * 1024 * 1024)
#define WORKER_COUNT 4

typedef struct {
	Static_Arena arena;
	Pull_Requests pull_requests;
	bool loaded;
} Repository_State;

typedef struct {
	Repository_State repositories[MAX_REPOSITORIES];
	GtkWidget *window;
	GtkWidget *split_view;
	AdwNavigationPage *content_page;
	GtkWidget *clamp;
	usize selected_repo;
	_Atomic bool ready;
} App_State;

typedef struct {
	usize id;
	usize count;
} Worker_Context;

global_variable App_State app_state;
global_variable _Alignas(max_align_t) u8 pr_storage[PR_STORAGE_CAPACITY];
global_variable Config_State config;
global_variable char config_path[4096];
global_variable pthread_t workers[WORKER_COUNT];
global_variable Worker_Context worker_contexts[WORKER_COUNT];
global_variable _Atomic usize workers_done;
global_variable usize worker_count;
global_variable usize arena_capacity;

internal GtkWidget *pull_request_list_new(const Pull_Requests *requests) {
	GtkWidget *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	for (usize i = 0; i < requests->len; i += 1) {
		Pull_Request_Card_Data data = {
			.number = requests->number[i],
			.title = requests->title[i],
			.author = requests->author[i],
			.url = requests->url[i],
			.is_draft = requests->is_draft[i],
		};

		gtk_box_append(GTK_BOX(list), pull_request_card_new(data));
	}

	return list;
}

internal void show_selected_repository(App_State *state) {
	GtkWidget *content;
	if (config.len == 0) {
		content = gtk_label_new("No repositories configured.");
	} else if (!atomic_load_explicit(&state->ready, memory_order_acquire)) {
		content = gtk_label_new("Loading repositories...");
	} else {
		Repository_State *repo = &state->repositories[state->selected_repo];
		if (repo->loaded && repo->pull_requests.len > 0) {
			content = pull_request_list_new(&repo->pull_requests);
		} else if (repo->loaded) {
			content = gtk_label_new("No open pull requests.");
		} else {
			content = gtk_label_new("Could not load pull requests. Check gh authentication and try again.");
			gtk_label_set_wrap(GTK_LABEL(content), true);
		}
	}

	gtk_widget_set_margin_top(content, 18);
	gtk_widget_set_margin_bottom(content, 18);
	gtk_widget_set_margin_start(content, 18);
	gtk_widget_set_margin_end(content, 18);
	adw_clamp_set_child(ADW_CLAMP(state->clamp), content);
}

internal void on_repository_selected(AdwSidebar *sidebar, GParamSpec *pspec, void *user_data) {
	(void)pspec;
	guint const selected = adw_sidebar_get_selected(sidebar);
	if (selected == GTK_INVALID_LIST_POSITION) {
		return;
	}
	App_State *state = user_data;
	state->selected_repo = selected;
	Config_Repository repo = config.repositories[state->selected_repo];
	char name[512];
	snprintf(name, sizeof name, "%.*s/%.*s", (int)repo.owner.len, repo.owner.str,
		(int)repo.repo.len, repo.repo.str);
	adw_navigation_page_set_title(state->content_page, name);
	show_selected_repository(state);
}

internal void on_repository_activated(AdwSidebar *sidebar, guint index, void *user_data) {
	(void)sidebar;
	(void)index;
	App_State *state = user_data;
	adw_navigation_split_view_set_show_content(ADW_NAVIGATION_SPLIT_VIEW(state->split_view), true);
}

internal gboolean on_fetches_ready(void *user_data) {
	App_State *state = user_data;
	if (!atomic_load_explicit(&state->ready, memory_order_acquire)) {
		return G_SOURCE_CONTINUE;
	}
	show_selected_repository(state);
	return G_SOURCE_REMOVE;
}

internal void *fetch_repositories(void *user_data) {
	Worker_Context *worker = user_data;
	for (usize i = worker->id; i < config.len; i += worker->count) {
		Repository_State *state = &app_state.repositories[i];
		state->arena = static_arena_init(pr_storage + i * arena_capacity, arena_capacity);
		Config_Repository repo = config.repositories[i];
		String_View name = {.str = repo.owner.str, .len = repo.owner.len + 1 + repo.repo.len};
		state->loaded = pull_requests_get(&state->arena, &state->pull_requests, name);
	}

	if (atomic_fetch_add_explicit(&workers_done, 1, memory_order_acq_rel) + 1 == worker_count) {
		atomic_store_explicit(&app_state.ready, true, memory_order_release);
	}
	return NULL;
}

internal void on_activate(GtkApplication *app, void *user_data) {
	App_State *state = user_data;
	if (state->window != NULL) {
		gtk_window_present(GTK_WINDOW(state->window));
		return;
	}

	state->window = adw_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(state->window), "MRView");
	gtk_window_set_default_size(GTK_WINDOW(state->window), 900, 720);

	state->split_view = adw_navigation_split_view_new();
	GtkWidget *sidebar = adw_sidebar_new();
	AdwSidebarSection *section = adw_sidebar_section_new();
	adw_sidebar_section_set_title(section, "Repositories");
	for (usize i = 0; i < config.len; i += 1) {
		Config_Repository repo = config.repositories[i];
		char name[512];
		snprintf(name, sizeof name, "%.*s", (int)repo.repo.len, repo.repo.str);
		adw_sidebar_section_append(section, adw_sidebar_item_new(name));
	}
	adw_sidebar_append(ADW_SIDEBAR(sidebar), section);

	GtkWidget *sidebar_toolbar = adw_toolbar_view_new();
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(sidebar_toolbar), adw_header_bar_new());
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(sidebar_toolbar), sidebar);
	AdwNavigationPage *sidebar_page = adw_navigation_page_new(sidebar_toolbar, "MRView");
	adw_navigation_split_view_set_sidebar(ADW_NAVIGATION_SPLIT_VIEW(state->split_view), sidebar_page);

	GtkWidget *scrolled = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	state->clamp = adw_clamp_new();
	adw_clamp_set_maximum_size(ADW_CLAMP(state->clamp), 760);
	adw_clamp_set_tightening_threshold(ADW_CLAMP(state->clamp), 600);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), state->clamp);
	GtkWidget *content_toolbar = adw_toolbar_view_new();
	adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(content_toolbar), adw_header_bar_new());
	adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(content_toolbar), scrolled);
	state->content_page = adw_navigation_page_new(content_toolbar, "Pull requests");
	adw_navigation_split_view_set_content(ADW_NAVIGATION_SPLIT_VIEW(state->split_view), state->content_page);

	AdwBreakpoint *breakpoint = adw_breakpoint_new(adw_breakpoint_condition_parse("max-width: 600sp"));
	adw_breakpoint_add_setters(breakpoint,
		G_OBJECT(state->split_view), "collapsed", true,
		G_OBJECT(sidebar), "mode", ADW_SIDEBAR_MODE_PAGE,
		NULL);
	adw_application_window_add_breakpoint(ADW_APPLICATION_WINDOW(state->window), breakpoint);

	if (config.len > 0) {
		adw_sidebar_set_selected(ADW_SIDEBAR(sidebar), 0);
	}
	g_signal_connect(sidebar, "notify::selected", G_CALLBACK(on_repository_selected), state);
	g_signal_connect(sidebar, "activated", G_CALLBACK(on_repository_activated), state);
	if (config.len > 0) {
		on_repository_selected(ADW_SIDEBAR(sidebar), NULL, state);
	} else {
		show_selected_repository(state);
	}

	adw_application_window_set_content(ADW_APPLICATION_WINDOW(state->window), state->split_view);
	gtk_window_present(GTK_WINDOW(state->window));

	worker_count = config.len < WORKER_COUNT ? config.len : WORKER_COUNT;
	if (worker_count > 0) {
		arena_capacity = PR_STORAGE_CAPACITY / config.len;
		arena_capacity -= arena_capacity % _Alignof(max_align_t);
		for (usize i = 0; i < worker_count; i += 1) {
			worker_contexts[i] = (Worker_Context){.id = i, .count = worker_count};
			if (pthread_create(&workers[i], NULL, fetch_repositories, &worker_contexts[i]) != 0) {
				abort();
			}
		}
		g_timeout_add(50, on_fetches_ready, state);
	}
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

	g_autoptr(AdwApplication) app = adw_application_new("net.mooremoore.MRView", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(on_activate), &app_state);

	i32 status = g_application_run(G_APPLICATION(app), argc, argv);

	for (usize i = 0; i < worker_count; i += 1) {
		pthread_join(workers[i], NULL);
	}
	return status;
}
