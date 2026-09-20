#include <adwaita.h>
#include <stdio.h>
#include <string.h>

#include "base.h"
#include "string_view.h"
#include "prs.h"

typedef struct {
    char label_buf[1024];
    i32 counter;
} App_State;

App_State app_state = {0};

internal void on_counter_button_click(GtkWidget *button, void *user_data) {
    GtkLabel *label = GTK_LABEL(user_data);

    app_state.counter += 1;
    sprintf(app_state.label_buf, "%d", app_state.counter);
    gtk_label_set_text(label, app_state.label_buf);
}

internal void on_activate(GtkApplication *app) {
    char label_buf[1024] = "0";

    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *label  = gtk_label_new(label_buf);
    GtkWidget *button = gtk_button_new_with_label("Increment");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), button);

    g_signal_connect(button, "clicked", G_CALLBACK(on_counter_button_click), label);

    gtk_window_set_title(GTK_WINDOW(window), "Hello");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
    gtk_window_set_child(GTK_WINDOW(window), box);

    gtk_window_present(GTK_WINDOW(window));
}

i32 main(i32 argc, char *argv[]) {
    g_autoptr(AdwApplication) app = adw_application_new("net.mooremoore.practice", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    Arena arena = arena_init();
    Pull_Requests requests = {0};

    pull_requests_get_for_url(&arena, &requests, string_view_from_cstr("fakeurl"));

    return g_application_run(G_APPLICATION(app), argc, argv);
}
