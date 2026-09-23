#include "pull_request_card.h"

#include <stdio.h>
#include <stdbool.h>
#include <adwaita.h>
#include "ui_data/pull_request_card_ui.h"

GtkWidget *pull_request_card_new(Pull_Request_Card_Data data) {
	GtkBuilder    *builder;

	GtkWidget     *card;
	GtkLabel      *title;
	GtkLabel      *details;
	GtkLinkButton *open_link;

	char metadata[256];

	builder = gtk_builder_new_from_string((char const *)pull_request_card_ui, (usize)pull_request_card_ui_len);

	card      = GTK_WIDGET(gtk_builder_get_object(builder, "card"));
	title     = GTK_LABEL(gtk_builder_get_object(builder, "title"));
	details   = GTK_LABEL(gtk_builder_get_object(builder, "details"));
	open_link = GTK_LINK_BUTTON(gtk_builder_get_object(builder, "open_link"));

	gtk_label_set_text(title, data.title.str);

	if (data.author.len > 0) {
		snprintf(metadata, sizeof metadata, "#%d · %.*s", data.number, (int)data.author.len, data.author.str);
	} else {
		snprintf(metadata, sizeof metadata, "#%d · Unknown author", data.number);
	}

	if (data.is_draft) {
		gtk_widget_add_css_class(GTK_WIDGET(title),   "dimmed");
		gtk_widget_add_css_class(GTK_WIDGET(details), "dimmed");
	}

	gtk_label_set_text(details, metadata);
	gtk_link_button_set_uri(open_link, data.url.str);

	g_object_ref(card);
	g_object_unref(builder);

	return card;
}
