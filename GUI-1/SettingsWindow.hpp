#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include "MainWindow.h"
#include "LobbyPage.hpp"
#include "Settings.h"  // Assuming Settings is the class for XML management

class LobbyPage;
class MainWindow;

class SettingsWindow : public Fl_Window {
public:
    // Constructor
    SettingsWindow(int width, int height, const char* title, MainWindow* mainWindow, LobbyPage* lobbyPage);

    // Destructor
    ~SettingsWindow();

    // Apply settings changes to the main window
    void apply_changes();

private:
    // UI elements
    Fl_Input* username_input;
    Fl_Check_Button* theme_toggle;
    Fl_Choice* resolution_choice;
    Fl_Button* apply_button;
    Fl_Button* close_button;

    // Main window and lobby page (used for updates)
    MainWindow* mainWindow;
    LobbyPage* lobbyPage;

    // User settings
    std::string m_username;  // Current username
    Settings m_settings;     // Settings management object

    // Setup UI components (called by the constructor)
    void setup_ui();

    // Apply the resolution settings to the main window
    void apply_resolution();

    // Apply the dark mode setting
    void apply_dark_mode();
};

#endif // SETTINGSWINDOW_H
