#ifndef LOBBY_PAGE_HPP
#define LOBBY_PAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <string>
#include <vector>
#include <memory>

#include "ServerSocket.h"
#include "ClientSocket.h"

class LobbyPage : public Fl_Group {
private:
    Fl_Text_Display* chatDisplay;
    Fl_Text_Buffer* chatBuffer;
    Fl_Box* connectionStatusBox;
    Fl_Box* playerSidebar;
    Fl_Input* inputBox;

    std::shared_ptr<ServerSocket> serverSocket;
    std::shared_ptr<ClientSocket> clientSocket;

    std::string localUsername;
    std::vector<std::string> players;

    void handleServerUpdates();
    void handleClientUpdates();
    void broadcastMessage(const std::string& message, std::shared_ptr<ClientSocket> sender);
    void broadcastUserList();
    void updatePlayerList();
    void updatePlayerListFromMessage(const std::string& message);
    void addChatMessage(const std::string& message);
    static void send_button_callback(Fl_Widget* widget, void* userdata);

public:
    LobbyPage(int x, int y, int w, int h);

    void setServerSocket(std::shared_ptr<ServerSocket> server, const std::string& username);
    void setClientSocket(std::shared_ptr<ClientSocket> client, const std::string& username);
    void Update();
    void addPlayer(const std::string& playerName);
};

#endif // LOBBY_PAGE_HPP