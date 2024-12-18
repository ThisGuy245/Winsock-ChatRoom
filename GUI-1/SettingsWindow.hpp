#ifndef SETTINGSWINDOW_HPP
#define SETTINGSWINDOW_HPP

// FLTK 
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>

// My Pages
#include "MainWindow.h"
#include "LobbyPage.hpp"

class SettingsWindow : public Fl_Window {
public:
    SettingsWindow(int width, int height, const char* title, MainWindow* mainWindow, LobbyPage* lobbyPage);
    ~SettingsWindow();

    void apply_resolution();
    void apply_changes();
    void apply_dark_mode(LobbyPage* lobbyPage);

private:
    Fl_Input* username_input;       // Input box for username
    Fl_Check_Button* theme_toggle;  // Light/Dark mode switch
    Fl_Choice* resolution_choice;   // Dropdown for resolutions
    Fl_Button* apply_button;        // Apply button
    Fl_Button* close_button;        // Close button

    MainWindow* mainWindow;
    LobbyPage* lobbyPage;

    void setup_ui();                // Internal function to set up UI
};

#endif // SETTINGSWINDOW_HPP
