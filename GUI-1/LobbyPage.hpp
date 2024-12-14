#ifndef LOBBYPAGE_HPP
#define LOBBYPAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Input.H>
#include <vector>
#include <string>
#include <memory>
#include "ClientSocket.h"  // Include ClientSocket class definition if not included already
#include "ServerSocket.h"  // Include ServerSocket class definition if not included already

class LobbyPage : public Fl_Group {
public:
    // Constructor
    LobbyPage(int x, int y, int w, int h);

    // Update function called periodically for client-server communication
    void Update();

    // Initialize the server-side settings
    void initializeServer(std::shared_ptr<ServerSocket> server, const std::string& username);
    void updateConnectedUsers();
    // Initialize the client-side settings
    void initializeClient(std::shared_ptr<ClientSocket> client, const std::string& username);

private:
    // Helper functions for managing server-client communication
    void processServerUpdates();
    void processClientUpdates();
    void sendMessageToAll(const std::string& message, std::shared_ptr<ClientSocket> sender = nullptr);
    void sendUserList();
    void refreshUserList(const std::string& message);
    void appendChatMessage(const std::string& message);
    void applyStyling();
    void setClientSocket(localClientSocket, username);

    // Callback function for sending messages
    static void sendMessageCallback(Fl_Widget* widget, void* userdata);

    // UI components
    Fl_Text_Display* chatDisplay;
    Fl_Text_Buffer* chatBuffer;
    Fl_Text_Display* userListDisplay;
    Fl_Text_Buffer* userListBuffer;
    Fl_Input* inputBox;

    // Networking components
    std::shared_ptr<ClientSocket> clientSocket;
    std::shared_ptr<ServerSocket> serverSocket;
    std::vector<std::string> players;
    std::string localUsername;
};

#endif // LOBBYPAGE_HPP
