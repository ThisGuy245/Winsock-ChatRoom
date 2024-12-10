#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Display.H>

// Forward declaration of MainWindow to avoid circular dependency
class MainWindow;
class LobbyPage;

class HomePage : public Fl_Group {
public:
    // Constructor accepting MainWindow pointer
    HomePage(int x, int y, int w, int h, MainWindow* mainWindow);
    ~HomePage();

    static void host_button_callback(Fl_Widget* widget, void* userdata);
    static void join_button_callback(Fl_Widget* widget, void* userdata);
    static void login_button_callback(Fl_Widget* widget, void* userdata);  // Callback for login button
    static void username_input_callback(Fl_Widget* widget, void* userdata);  // Callback for username input

    LobbyPage* lobbyPage;

private:
    MainWindow* m_mainWindow; // Pointer to the parent MainWindow
    Fl_Input* usernameInput;  // Input field for the username
    Fl_Button* hostButton;    // Button to host the server
    Fl_Button* joinButton;    // Button to join the server
    Fl_Button* loginButton;   // Login button to activate Host/Join buttons

    // Static callback functions
    
};

#endif
