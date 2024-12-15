// HomePage.hpp
#ifndef HOMEPAGE_HPP
#define HOMEPAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <string>
#include "MainWindow.h"
#include "LobbyPage.hpp"

class MainWindow;

class HomePage : public Fl_Group {
public:
    HomePage(int X, int Y, int W, int H, MainWindow* parent);
    ~HomePage();

    // Callback functions for host and join buttons
    static void hostButtonCallback(Fl_Widget* widget, void* userdata);
    static void joinButtonCallback(Fl_Widget* widget, void* userdata);

private:
    Fl_Input* usernameInput;
    Fl_Input* ipInput;
    Fl_Button* hostButton;
    Fl_Button* joinButton;
    MainWindow* mainWindow;

    std::string getLocalIPAddress();
};

#endif // HOMEPAGE_HPP
