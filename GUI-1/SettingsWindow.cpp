#include "SettingsWindow.hpp"
#include "MainWindow.h"
#include "LobbyPage.hpp"

#include <cstdio>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>

SettingsWindow::SettingsWindow(int width, int height, const char* title, MainWindow* mainWindow, LobbyPage* lobbyPage)
    : Fl_Window(width, height, title), mainWindow(mainWindow) { // Pass MainWindow instance
    setup_ui();
    this->end(); // Finalize the window
}

SettingsWindow::~SettingsWindow() {
    // No dynamic memory allocation here, FLTK handles widget cleanup
}

void SettingsWindow::setup_ui() {
    // Username input section
    Fl_Box* username_label = new Fl_Box(20, 20, 100, 30, "Change Username:");
    username_input = new Fl_Input(130, 20, 200, 30, ""); // Placeholder input box

    // Light/Dark mode toggle
    theme_toggle = new Fl_Check_Button(20, 70, 150, 30, "Enable Dark Mode");
    theme_toggle->box(FL_FLAT_BOX);
    theme_toggle->down_box(FL_ROUND_DOWN_BOX);

    // Resolution dropdown
    Fl_Box* resolution_label = new Fl_Box(20, 120, 100, 30, "Resolution:");
    resolution_choice = new Fl_Choice(130, 120, 200, 30);
    resolution_choice->add("800x600|1024x768|1280x720|1920x1080|Custom");
    resolution_choice->value(4); // Set default to Custom

    // Apply button
    apply_button = new Fl_Button(50, 180, 100, 30, "Apply");
    apply_button->color(FL_DARK_GREEN);
    apply_button->labelcolor(FL_WHITE);
    apply_button->callback([](Fl_Widget*, void* data) {
        Fl_Window* window = (Fl_Window*)data;
        auto* settings = static_cast<SettingsWindow*>(data);
        settings->apply_changes();  // Apply all settings
        window->hide();
        }, this);

    // Close button
    close_button = new Fl_Button(170, 180, 100, 30, "Close");
    close_button->color(FL_DARK_RED);
    close_button->labelcolor(FL_WHITE);
    close_button->callback([](Fl_Widget* w, void* data) {
        Fl_Window* window = (Fl_Window*)data;
        window->hide(); // Close the settings window
        }, this);
}

void SettingsWindow::apply_changes() {
    apply_resolution();
    apply_dark_mode(lobbyPage);

    const char* newUsername = username_input->value();
    if (newUsername != "") {
        printf(newUsername);
        lobbyPage->changeUsername(newUsername);
        return;
    }
}

void SettingsWindow::apply_resolution() {
    int selected = resolution_choice->value();
    int width = 800, height = 600;

    switch (selected) {
    case 0: width = 800; height = 600; break;
    case 1: width = 1024; height = 768; break;
    case 3: width = 1920; height = 1080; break;
    case 4: width = mainWindow->w(); height = mainWindow->h(); break;
    }

    mainWindow->setResolution(width, height);
}

void SettingsWindow::apply_dark_mode(LobbyPage* lobbyPage) {
    bool isDarkMode = theme_toggle->value();

    // Set global background and foreground colors
    Fl::background(isDarkMode ? 45 : 240, isDarkMode ? 45 : 240, isDarkMode ? 45 : 240);
    Fl::foreground(isDarkMode ? 255 : 0, isDarkMode ? 255 : 0, isDarkMode ? 255 : 0);

    mainWindow->redraw();

    if (!lobbyPage) return;

    // Apply grey background for scroll area
    if (lobbyPage->scrollArea) {
        lobbyPage->scrollArea->color(fl_rgb_color(60, 60, 60)); // Dark grey
        lobbyPage->scrollArea->redraw();
    }

    // Apply grey background and white text for chat display
    if (lobbyPage->chatDisplay) {
        lobbyPage->chatDisplay->color(fl_rgb_color(50, 50, 50));  // Dark grey
        lobbyPage->chatDisplay->textcolor(FL_WHITE);              // White text
        lobbyPage->chatDisplay->redraw();
    }

    // Apply grey background and white text for message input
    if (lobbyPage->messageInput) {
        lobbyPage->messageInput->color(fl_rgb_color(45, 45, 45)); // Slightly darker grey
        lobbyPage->messageInput->textcolor(FL_WHITE);            // White text
        lobbyPage->messageInput->redraw();
    }

    // Apply grey background to player display if applicable
    if (lobbyPage->playerDisplay) {
        lobbyPage->playerDisplay->color(fl_rgb_color(60, 60, 60));
        lobbyPage->playerDisplay->redraw();
    }
}
