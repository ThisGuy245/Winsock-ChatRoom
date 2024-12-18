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

// Constructor initializes the SettingsWindow and sets up the UI
SettingsWindow::SettingsWindow(int width, int height, const char* title, MainWindow* mainWindow, LobbyPage* lobbyPage)
    : Fl_Window(width, height, title), mainWindow(mainWindow), lobbyPage(lobbyPage) {  // Initialize lobbyPage here
    setup_ui();
    this->end();  // Finalize the window
}

// Destructor (no need for manual cleanup since FLTK manages widget memory)
SettingsWindow::~SettingsWindow() {
}

// Setup the UI components for the settings window
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
        // Directly call apply_changes here
        auto* settings = static_cast<SettingsWindow*>(data);
        settings->apply_changes();
        settings->hide();
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

// Apply changes based on user input (resolution, dark mode, and username)
void SettingsWindow::apply_changes() {
    // Apply changes only if the lobbyPage is not null
    if (lobbyPage) {
        apply_resolution();
        apply_dark_mode();
        //lobbyPage->applyStyles();

        const char* newUsername = username_input->value();
        if (newUsername && newUsername[0] != '\0') {
            if (lobbyPage) {
                lobbyPage->changeUsername(newUsername);  // Change username if lobbyPage is valid
            }
        }
    }
}


// Apply selected resolution to the main window
void SettingsWindow::apply_resolution() {
    int selected = resolution_choice->value();
    int width = 800, height = 600;

    switch (selected) {
    case 0: width = 800; height = 600; break;
    case 1: width = 1024; height = 768; break;
    case 3: width = 1920; height = 1080; break;
    case 4: width = mainWindow->w(); height = mainWindow->h(); break;
    }

    mainWindow->setResolution(width, height);  // Update main window resolution
}

// Apply dark mode settings to the lobby page and other widgets
void SettingsWindow::apply_dark_mode() {
    bool isDarkMode = theme_toggle->value();

    // Set global background and foreground colors for the main window
    if (isDarkMode) {
        Fl::background(45, 45, 45); // Dark background for main window
        Fl::foreground(255, 255, 255); // White text
    }
    else {
        Fl::background(240, 240, 240); // Light background
        Fl::foreground(0, 0, 0); // Black text
    }
    mainWindow->redraw();  // Redraw the main window to apply changes

    // Ensure lobbyPage is not nullptr before applying styles
    if (!lobbyPage) return;

    // Apply dark/light mode styles to components in the lobby page
    if (lobbyPage->scrollArea) {
        if (isDarkMode) {
            lobbyPage->scrollArea->color(fl_rgb_color(60, 60, 60)); // Dark grey
        }
        else {
            lobbyPage->scrollArea->color(fl_rgb_color(255, 255, 255)); // Light grey
        }
        lobbyPage->scrollArea->redraw();
    }

    if (lobbyPage->chatDisplay) {
        if (isDarkMode) {
            lobbyPage->chatDisplay->color(fl_rgb_color(50, 50, 50)); // Dark grey
            lobbyPage->chatDisplay->textcolor(FL_WHITE);  // White text
        }
        else {
            lobbyPage->chatDisplay->color(fl_rgb_color(255, 255, 255)); // White background
            lobbyPage->chatDisplay->textcolor(FL_BLACK);  // Black text
        }
        lobbyPage->chatDisplay->redraw();
    }

    if (lobbyPage->messageInput) {
        if (isDarkMode) {
            lobbyPage->messageInput->color(fl_rgb_color(45, 45, 45)); // Dark grey
            lobbyPage->messageInput->textcolor(FL_WHITE);  // White text
        }
        else {
            lobbyPage->messageInput->color(fl_rgb_color(255, 255, 255)); // Light background
            lobbyPage->messageInput->textcolor(FL_BLACK);  // Black text
        }
        lobbyPage->messageInput->redraw();
    }

    if (lobbyPage->playerDisplay) {
        if (isDarkMode) {
            lobbyPage->playerDisplay->disp->color(fl_rgb_color(60, 60, 60)); // Dark grey
        }
        else {
            lobbyPage->playerDisplay->disp->color(fl_rgb_color(255, 255, 255)); // Light background
        }
        lobbyPage->playerDisplay->redraw();
    }
}
