#include "pull_request_card.h"

#include <stdio.h>

GtkWidget *pull_request_card_new(Pull_Request_Card_Data data) {
	GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_widget_add_css_class(card, "card");
	gtk_widget_set_margin_top(card, 6);
	gtk_widget_set_margin_bottom(card, 6);
	gtk_widget_set_margin_start(card, 6);
	gtk_widget_set_margin_end(card, 6);

	GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_widget_set_margin_top(content, 16);
	gtk_widget_set_margin_bottom(content, 16);
	gtk_widget_set_margin_start(content, 16);
	gtk_widget_set_margin_end(content, 16);
	gtk_box_append(GTK_BOX(card), content);

	GtkWidget *title = gtk_label_new(data.title.str);
	gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
	gtk_label_set_wrap(GTK_LABEL(title), true);
	gtk_label_set_wrap_mode(GTK_LABEL(title), PANGO_WRAP_WORD_CHAR);
	gtk_label_set_lines(GTK_LABEL(title), 2);
	gtk_widget_add_css_class(title, "title-3");
	gtk_box_append(GTK_BOX(content), title);

	char metadata[256];
	if (data.author.len > 0) {
		snprintf(metadata, sizeof metadata, "#%d · %.*s", data.number, (int)data.author.len, data.author.str);
	} else {
		snprintf(metadata, sizeof metadata, "#%d · Unknown author", data.number);
	}

	GtkWidget *details = gtk_label_new(metadata);
	gtk_label_set_xalign(GTK_LABEL(details), 0.0f);
	gtk_widget_add_css_class(details, "dim-label");
	gtk_box_append(GTK_BOX(content), details);

	// GtkWidget *description = gtk_label_new(data.description.len > 0 ? data.description.str : "No description provided.");
	// gtk_label_set_xalign(GTK_LABEL(description), 0.0f);
	// gtk_label_set_wrap(GTK_LABEL(description), true);
	// gtk_label_set_wrap_mode(GTK_LABEL(description), PANGO_WRAP_WORD_CHAR);
	// gtk_label_set_ellipsize(GTK_LABEL(description), PANGO_ELLIPSIZE_END);
	// gtk_label_set_lines(GTK_LABEL(description), 3);
	// gtk_box_append(GTK_BOX(content), description);

	GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_halign(footer, GTK_ALIGN_END);
	gtk_box_append(GTK_BOX(content), footer);

	GtkWidget *open = gtk_link_button_new_with_label(data.url.str, "Open on GitHub");
	gtk_widget_add_css_class(open, "flat");
	gtk_box_append(GTK_BOX(footer), open);

	return card;
}
