#ifndef LOBBYPAGE_HPP
#define LOBBYPAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

#include <string>

#include "ClientSocket.h"
#include "ServerSocket.h"

class LobbyPage : public Fl_Group {
public:
    LobbyPage(int X, int Y, int W, int H);
    ~LobbyPage();

    void hostServer(const std::string& ip, const std::string& username);
    void joinServer(const std::string& ip, const std::string& username);
    void Update();  // Network update
    void sendMessage(const std::string& message);  // Send a message to the server
    void receiveMessages();  // Helper function to process received messages

private:
    Fl_Input* messageInput;
    Fl_Button* sendButton;
    ClientSocket* client;
    ServerSocket* server;
    Fl_Text_Display* chatDisplay;  // Display for chat history
};

#endif // LOBBYPAGE_HPP
