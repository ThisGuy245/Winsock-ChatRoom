#ifndef LOBBYPAGE_HPP
#define LOBBYPAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Scroll.H>

#include <string>
#include "ClientSocket.h"
#include "ServerSocket.h"
#include "PlayerDisplay.hpp"
#include "SettingsWindow.hpp" // Include SettingsWindow header after the forward declaration
#include "AboutWindow.h"

// Forward declaration for SettingsWindow class
class SettingsWindow;

class LobbyPage : public Fl_Group {
public:
    LobbyPage(int X, int Y, int W, int H);
    ~LobbyPage();

    void hostServer(const std::string& ip, const std::string& username);
    void joinServer(const std::string& ip, const std::string& username);
    void Update();  // Network update
    void sendMessage(const std::string& message);
    void receiveMessages();
    void setUsername(const std::string& user) { username = user; }
    void clientLeft(const std::string& username);
    void resizeWidgets(int X, int Y, int W, int H); // Resize widgets
    void changeUsername(const std::string& newUsername);

    // Widgets
    Fl_Menu_Bar* menuBar;
    Fl_Scroll* scrollArea;
    Fl_Text_Display* chatDisplay;
    Fl_Text_Buffer* chatBuffer;
    Fl_Input* messageInput;
    Fl_Button* sendButton;

    PlayerDisplay* playerDisplay;
    ClientSocket* client;
    ServerSocket* server;
    SettingsWindow* settings;
    AboutWindow* about;

private:
    std::string username;

    // Callback for menu items
    static void menuCallback(Fl_Widget* widget, void* userdata);
};

#endif // LOBBYPAGE_HPP
