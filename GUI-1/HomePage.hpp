#ifndef HOMEPAGE_HPP
#define HOMEPAGE_HPP

#include <FL/Fl_Group.H>
#include <string>

class Fl_Input;
class Fl_Button;
class MainWindow;

class HomePage : public Fl_Group {
public:
    // Constructor
    HomePage(int X, int Y, int W, int H, MainWindow* parent);

    // Destructor
    virtual ~HomePage();

private:
    MainWindow* mainWindow;  // Reference to the parent MainWindow

    Fl_Input* usernameInput; // Input field for username
    Fl_Input* ipInput;       // Input field for server IP address
    Fl_Button* hostButton;   // Button to host a server
    Fl_Button* joinButton;   // Button to join a server

    // Callback functions for the buttons
    static void hostButtonCallback(Fl_Widget* widget, void* userdata);
    static void joinButtonCallback(Fl_Widget* widget, void* userdata);

    // Utility function to get the local IP address
    static std::string getLocalIPAddress();
};

#endif // HOMEPAGE_HPP