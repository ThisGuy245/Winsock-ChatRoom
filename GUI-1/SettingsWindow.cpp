#include "SettingsWindow.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

SettingsWindow::SettingsWindow(int width, int height, const char* title)
    : Fl_Window(width, height, title) {
    setup_ui();
    this->end(); // Finalize the window
}

SettingsWindow::~SettingsWindow() {
    // No dynamic memory allocation here, FLTK handles widget cleanup
}

void SettingsWindow::setup_ui() {
    // Username input section
    Fl_Box* username_label = new Fl_Box(20, 20, 100, 30, "Username:");
    username_input = new Fl_Input(130, 20, 200, 30, ""); // Placeholder input box

    // Light/Dark mode toggle
    theme_toggle = new Fl_Check_Button(20, 70, 150, 30, "Enable Dark Mode");
    theme_toggle->box(FL_FLAT_BOX);
    theme_toggle->down_box(FL_ROUND_DOWN_BOX);

    // Resolution dropdown
    Fl_Box* resolution_label = new Fl_Box(20, 120, 100, 30, "Resolution:");
    resolution_choice = new Fl_Choice(130, 120, 200, 30);
    resolution_choice->add("800x600|1024x768|1280x720|1920x1080|Custom");
    resolution_choice->value(3); // Set default to 1920x1080

    // Apply button
    apply_button = new Fl_Button(50, 180, 100, 30, "Apply");
    apply_button->color(FL_DARK_GREEN);
    apply_button->labelcolor(FL_WHITE);

    // Close button
    close_button = new Fl_Button(170, 180, 100, 30, "Close");
    close_button->color(FL_DARK_RED);
    close_button->labelcolor(FL_WHITE);
    close_button->callback([](Fl_Widget* w, void* data) {
        Fl_Window* window = (Fl_Window*)data;
        window->hide(); // Close the settings window
        }, this);
}
