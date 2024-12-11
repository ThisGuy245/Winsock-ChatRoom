#ifndef LOBBY_PAGE_HPP
#define LOBBY_PAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <string>
#include <vector>
#include <memory>
#include "ServerSocket.h"
#include "ClientSocket.h"

class LobbyPage : public Fl_Group {
private:
    Fl_Text_Display* chatDisplay;
    Fl_Text_Buffer* chatBuffer;
    Fl_Input* inputBox;
    Fl_Box* playerSidebar;
    Fl_Box* connectionStatusBox;

    std::shared_ptr<ServerSocket> serverSocket;
    std::shared_ptr<ClientSocket> clientSocket;

    std::string localUsername;
    std::vector<std::string> players;

    static void send_button_callback(Fl_Widget* widget, void* userdata);
    void handleServerUpdates();
    void handleClientUpdates();
    void addPlayer(const std::string& playerName);
    void updatePlayerList();

public:
    LobbyPage(int x, int y, int w, int h);
    void setServerSocket(std::shared_ptr<ServerSocket> server, const std::string& username);
    void setClientSocket(std::shared_ptr<ClientSocket> client, const std::string& username);
    void Update();
    void broadcastMessage(const std::string& message, std::shared_ptr<ClientSocket> sender = nullptr);
    void broadcastUserList();
    void addChatMessage(const std::string& message);
};

#endif // LOBBY_PAGE_HPP
