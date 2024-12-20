#include "SettingsWindow.hpp"
#include "MainWindow.h"
#include "LobbyPage.hpp"
#include <cstdio>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>

SettingsWindow::SettingsWindow(int width, int height, const char* title, MainWindow* mainWindow, LobbyPage* lobbyPage)
    : Fl_Window(width, height, title), mainWindow(mainWindow), lobbyPage(lobbyPage),
    m_settings("config.xml") // Specify the correct file path
{
    // Set the username to the current stored username in settings or default
    m_username = m_settings.getUsername(); // Get the username from settings

    setup_ui();
    this->end();  // Finalize the window
}


SettingsWindow::~SettingsWindow() {
    // Destructor, handled by FLTK automatically
}

void SettingsWindow::setup_ui() {
    // Username input section
    Fl_Box* username_label = new Fl_Box(20, 20, 100, 30, "Change Username:");
    username_input = new Fl_Input(130, 20, 200, 30, "");
    username_input->value(m_username.c_str());  // Pre-set the input with the current username

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

void SettingsWindow::apply_changes() {
    if (lobbyPage) {
        apply_resolution();
        apply_dark_mode();

        // Update username
        const char* newUsername = username_input->value();
        if (newUsername && newUsername[0] != '\0') {
            m_username = newUsername;  // Update username in the current session
            lobbyPage->changeUsername(newUsername);  // Update the lobby page
            m_settings.setUsername(newUsername);  // Update the username in settings (XML)
            m_settings.saveSettings();  // Save settings to file
        }
    }
}

void SettingsWindow::apply_resolution() {
    int selected = resolution_choice->value();
    int width = 800, height = 600;

    switch (selected) {
    case 0: width = 800; height = 600; break;
    case 1: width = 1024; height = 768; break;
    case 2: width = 1280; height = 720; break;
    case 3: width = 1920; height = 1080; break;
    case 4: width = mainWindow->w(); height = mainWindow->h(); break;
    }

    mainWindow->setResolution(width, height);

    // Update the resolution in the XML
    m_settings.setRes(m_username, width, height);
}

void SettingsWindow::apply_dark_mode() {
    bool isDarkMode = theme_toggle->value();

    // Apply dark/light mode to main window
    if (isDarkMode) {
        Fl::background(45, 45, 45); // Dark background for main window
        Fl::foreground(255, 255, 255); // White text
    }
    else {
        Fl::background(240, 240, 240); // Light background
        Fl::foreground(0, 0, 0); // Black text
    }
    mainWindow->redraw();

    // Apply dark/light mode to lobby page components
    if (lobbyPage) {
        if (lobbyPage->scrollArea) {
            lobbyPage->scrollArea->color(isDarkMode ? fl_rgb_color(60, 60, 60) : fl_rgb_color(255, 255, 255));
            lobbyPage->scrollArea->redraw();
        }
        if (lobbyPage->chatDisplay) {
            lobbyPage->chatDisplay->color(isDarkMode ? fl_rgb_color(50, 50, 50) : fl_rgb_color(255, 255, 255));
            lobbyPage->chatDisplay->textcolor(isDarkMode ? FL_WHITE : FL_BLACK);
            lobbyPage->chatDisplay->redraw();
        }
        if (lobbyPage->messageInput) {
            lobbyPage->messageInput->color(isDarkMode ? fl_rgb_color(45, 45, 45) : fl_rgb_color(255, 255, 255));
            lobbyPage->messageInput->textcolor(isDarkMode ? FL_WHITE : FL_BLACK);
            lobbyPage->messageInput->redraw();
        }
        if (lobbyPage->playerDisplay) {
            lobbyPage->playerDisplay->disp->color(isDarkMode ? fl_rgb_color(60, 60, 60) : fl_rgb_color(255, 255, 255));
            lobbyPage->playerDisplay->redraw();
        }
    }
}
