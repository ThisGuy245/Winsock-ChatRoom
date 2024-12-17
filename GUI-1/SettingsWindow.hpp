#ifndef SETTINGSWINDOW_HPP
#define SETTINGSWINDOW_HPP

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include "MainWindow.h"

class SettingsWindow : public Fl_Window {
public:
    SettingsWindow(int width, int height, const char* title, MainWindow* mainWindow);
    ~SettingsWindow();
    void apply_resolution();

private:
    Fl_Input* username_input;       // Input box for username
    Fl_Check_Button* theme_toggle; // Light/Dark mode switch
    Fl_Choice* resolution_choice;  // Dropdown for resolutions
    Fl_Button* apply_button;       // Apply button
    Fl_Button* close_button;       // Close button

    MainWindow* mainWindow;

    void setup_ui();               // Internal function to set up UI
};

#endif // SETTINGSWINDOW_HPP
