#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <string>
#include <vector>
#include <memory>
#include "ServerSocket.h"
#include "ClientSocket.h"

class LobbyPage : public Fl_Group {
public:
    LobbyPage(int x, int y, int w, int h);
    ~LobbyPage();

    void addPlayer(const std::string& playerName);   // Add a player to the list
    void addChatMessage(const std::string& message); // Add a message to the chat
    void removePlayer(const std::string& playerName);

    // Setters for networking objects
    void setServerSocket(std::shared_ptr<ServerSocket> server);
    void setClientSocket(std::shared_ptr<ClientSocket> client);

    // Getter for clientSocket
    std::shared_ptr<ClientSocket> getClientSocket() const;

private:
    Fl_Text_Display* chatDisplay;     // Chat message display
    Fl_Text_Buffer* chatBuffer;       // Buffer for chat messages
    Fl_Input* inputBox;               // Input box for chat
    Fl_Box* playerSidebar;            // Player list sidebar
    std::vector<std::string> players; // List of player names

    std::shared_ptr<ServerSocket> serverSocket; // Server socket reference
    std::shared_ptr<ClientSocket> clientSocket; // Client socket reference

    static void send_button_callback(Fl_Widget* widget, void* userdata); // Callback for send button
    void updatePlayerList(); // Refresh the player list display
};
