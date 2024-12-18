#ifndef SETTINGSWINDOW_HPP
#define SETTINGSWINDOW_HPP

// Forward declarations
class LobbyPage;    // Forward declaration of LobbyPage
class MainWindow;   // Forward declaration of MainWindow

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
    LobbyPage* lobbyPage;    // Reference to LobbyPage, will be used directly
    MainWindow* mainWindow;  // Reference to MainWindow

    // Constructor initializes the window with references to MainWindow and LobbyPage
    SettingsWindow(int width, int height, const char* title, MainWindow* mainWindow, LobbyPage* lobbyPage);
    ~SettingsWindow();

    // Apply changes based on user input
    void apply_resolution();
    void apply_changes();  // Removed argument here since lobbyPage is stored as a member
    void apply_dark_mode();

    Fl_Check_Button* theme_toggle;  // Light/Dark mode switch
    bool getThemeToggleState() const {
        return theme_toggle->value();
    }

private:
    Fl_Input* username_input;       // Input box for username
    Fl_Choice* resolution_choice;   // Dropdown for resolutions
    Fl_Button* apply_button;        // Apply button
    Fl_Button* close_button;        // Close button

    void setup_ui();         // Internal function to set up UI
};

#endif // SETTINGSWINDOW_HPP
