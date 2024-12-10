#ifndef LOBBY_PAGE_HPP
#define LOBBY_PAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <vector>
#include <memory>
#include <string>
#include "ChatData.hpp"
#include "ServerSocket.h"
#include "ClientSocket.h"

class LobbyPage : public Fl_Group {
public:
    LobbyPage(int x, int y, int w, int h);

    void setServerSocket(std::shared_ptr<ServerSocket> server, const std::string& username);
    void setClientSocket(std::shared_ptr<ClientSocket> client, const std::string& username);
    void Update();

    std::shared_ptr<ClientSocket> getClientSocket() const { return clientSocket; };
    void addChatMessage(const std::string& message);
private:
    static void send_button_callback(Fl_Widget* widget, void* userdata);

    void broadcastMessage(const std::string& message);
    void addPlayer(const std::string& playerName);
    void updatePlayerList();
    

    std::shared_ptr<ChatData> chatData;
    std::shared_ptr<ServerSocket> serverSocket;
    std::shared_ptr<ClientSocket> clientSocket;

    std::vector<std::string> players;
    std::string localUsername;

    Fl_Text_Display* chatDisplay;
    Fl_Text_Buffer* chatBuffer;
    Fl_Input* inputBox;
    Fl_Box* playerSidebar;
    Fl_Box* connectionStatusBox;
};

#endif