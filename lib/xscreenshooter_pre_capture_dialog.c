#include <gtk/gtk.h>
#include "xscreenshooter_debug.h"
#include "xscreenshooter_globals.h"
#include "xscreenshooter_utils.h"
#include "xscreenshooter_pre_capture_dialog.h"
#include "xscreenshooter_capture_utils.h"

void cb_entire_screen_radio_button_toggled(GtkToggleButton *self, CaptureData *capture_data)
{
	if (gtk_toggle_button_get_active(self))
		capture_data->capture_type = ENTIRE;
}

void cb_active_window_radio_button_toggled(GtkToggleButton *self, CaptureData *capture_data)
{
	if (gtk_toggle_button_get_active(self))
		capture_data->capture_type = ACTIVE;
}

void cb_select_area_radio_button_toggled(GtkToggleButton *self, CaptureData *capture_data)
{
	if (gtk_toggle_button_get_active(self))
		capture_data->capture_type = SELECT;
}

void cb_capture_cursor_check_button_toggled(GtkToggleButton *self, CaptureData *capture_data)
{
	if (gtk_toggle_button_get_active(self))
		capture_data->is_show_cursor = TRUE;
	else
		capture_data->is_show_cursor = FALSE;
}

void cb_delay_spin_button_value_changed(GtkSpinButton *self, CaptureData *capture_data)
{
	capture_data->delay = (gint)gtk_spin_button_get_value(self);
}

static void load_previous_session_values(
        GSList *radio_buttons, int previous_session_selection,
        GtkWidget *cursor_check_button, int previous_session_state, 
        GtkWidget *delay_spin_button, int previous_session_value)
{
    switch (previous_session_selection)
    {
        case SELECT:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_slist_nth_data(radio_buttons, 0)), TRUE);
            break;
        case ACTIVE:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_slist_nth_data(radio_buttons, 1)), TRUE);
            break;
        default: // ENTIRE
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_slist_nth_data(radio_buttons, 2)), TRUE);
            break;
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cursor_check_button), previous_session_state);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(delay_spin_button), previous_session_value);
}

GtkWidget *xscreenshooter_create_pre_capture_dialog(CaptureData *capture_data)
{
	GtkWidget *dialog, *header_bar, *grid, *label, *box, *options_box,
		  *radio_button, *check_button, *delay_box, *delay_spin_button_label, *delay_spin_button;

	dialog = gtk_dialog_new_with_buttons(
			NULL,
			NULL,
			GTK_DIALOG_MODAL,
			"_Cancel", GTK_RESPONSE_REJECT,
			"_Next", GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_widget_set_size_request(dialog, 490, 290);

	// CREATING AND SETTING THE HEADER
	header_bar = xscreenshooter_get_header_bar();
	gtk_window_set_titlebar(GTK_WINDOW(dialog), header_bar);


	// CREATING MAIN GRID FOR THE TWO SIDES
	grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(grid), 100);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid, FALSE, FALSE, 0);
	gtk_widget_set_margin_top(grid, 10);

	// BOX FOR LEFT SIDE WIDGETS
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_start(box, 15);
	gtk_grid_attach(GTK_GRID(grid), box, 0, 0, 1, 1);

	label = gtk_label_new("");
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Capture options</b>");
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

	// LEFT SIDE OPTIONS
	options_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_box_pack_start(GTK_BOX(box), options_box, TRUE, TRUE, 0);

	radio_button = gtk_radio_button_new_with_label(NULL, "Entire screen(s)");
	g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_entire_screen_radio_button_toggled), capture_data);
	gtk_box_pack_start(GTK_BOX(options_box), radio_button, FALSE, FALSE, 0);

	radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button), "Active window");
	g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_active_window_radio_button_toggled), capture_data);
	gtk_box_pack_start(GTK_BOX(options_box), radio_button, FALSE, FALSE, 0);

	radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button), "Select area");
	g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_select_area_radio_button_toggled), capture_data);
	gtk_box_pack_start(GTK_BOX(options_box), radio_button, FALSE, FALSE, 0);

	check_button = gtk_check_button_new_with_label("Capture cursor");
	g_signal_connect(check_button, "toggled", G_CALLBACK(cb_capture_cursor_check_button_toggled), capture_data);
	gtk_box_pack_start(GTK_BOX(options_box), check_button, FALSE, FALSE, 10);

	// BOX FOR RIGHT SIDE WIDGETS
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_start(box, 10);
	gtk_widget_set_margin_end(box, 15);
	gtk_container_add(GTK_CONTAINER(grid), box);

	label = gtk_label_new("");
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Delay</b>");
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

	delay_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_margin_end(delay_box, 35);
	gtk_widget_set_margin_start(delay_box, 35);
	gtk_box_pack_start(GTK_BOX(box), delay_box, FALSE, FALSE, 0);

	GtkAdjustment *delay_spin_button_adjustment = gtk_adjustment_new(0.0, 0.0, 99999999.0, 1.0, 5.0, 0.0);
	delay_spin_button = gtk_spin_button_new(delay_spin_button_adjustment, 1, 0);
	g_signal_connect(delay_spin_button, "value_changed", G_CALLBACK(cb_delay_spin_button_value_changed), capture_data);
	gtk_box_pack_start(GTK_BOX(delay_box), delay_spin_button, FALSE, FALSE, 0);

	delay_spin_button_label = gtk_label_new("seconds");
	gtk_box_pack_start(GTK_BOX(delay_box), delay_spin_button_label, FALSE, FALSE, 0);

    load_previous_session_values(gtk_radio_button_get_group(
            GTK_RADIO_BUTTON(radio_button)), capture_data->capture_type, 
            check_button, capture_data->is_show_cursor,
            delay_spin_button, capture_data->delay);

	return dialog;
}
