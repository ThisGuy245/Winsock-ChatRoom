#include "LobbyPage.hpp"
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

LobbyPage::LobbyPage(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), chatDisplay(nullptr), chatBuffer(nullptr), inputBox(nullptr) {
    // Create UI elements
    Fl_Box* navBar = new Fl_Box(x, y, w, 50, "");
    navBar->box(FL_FLAT_BOX);
    navBar->color(FL_BLUE);

    Fl_Box* titleBox = new Fl_Box(x + 10, y + 10, 200, 30, "Lobby Page");
    titleBox->labelsize(18);
    titleBox->align(FL_ALIGN_CENTER);

    connectionStatusBox = new Fl_Box(x + w - 150, y + 10, 140, 30, "Disconnected");
    connectionStatusBox->labelsize(14);
    connectionStatusBox->align(FL_ALIGN_CENTER);

    playerSidebar = new Fl_Box(x, y + 50, 200, h - 100, "Players:\n");
    playerSidebar->box(FL_UP_BOX);
    playerSidebar->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    chatBuffer = new Fl_Text_Buffer();
    chatDisplay = new Fl_Text_Display(x + 220, y + 60, w - 240, h - 150);
    chatDisplay->box(FL_BORDER_BOX);
    chatDisplay->buffer(chatBuffer);

    inputBox = new Fl_Input(x + 220, y + h - 90, w - 320, 30, "Message:");
    inputBox->align(FL_ALIGN_TOP);

    Fl_Button* sendButton = new Fl_Button(x + w - 100, y + h - 90, 80, 30, "Send");
    sendButton->callback(send_button_callback, this);

    end(); // End FLTK group
}

void LobbyPage::setServerSocket(std::shared_ptr<ServerSocket> server, const std::string& username) {
    serverSocket = server;
    localUsername = username;
    connectionStatusBox->label("Connected as Host");
    addPlayer(localUsername);
    broadcastMessage(localUsername + " has hosted the chat.", nullptr);
    broadcastUserList();
}

void LobbyPage::setClientSocket(std::shared_ptr<ClientSocket> client, const std::string& username) {
    clientSocket = client;
    localUsername = username;
    connectionStatusBox->label("Connected as Client");
    addPlayer(localUsername);
    broadcastMessage(localUsername + " has hosted the chat.", nullptr);
    broadcastUserList();
}

void LobbyPage::Update() {
    if (serverSocket) {
        handleServerUpdates();
    }
    if (clientSocket) {
        handleClientUpdates();
    }
}

void LobbyPage::handleServerUpdates() {
    auto newClient = serverSocket->accept();
    if (newClient) {
        serverSocket->addClient(newClient);
        broadcastUserList();
    }

    for (auto& client : serverSocket->getClients()) {
        std::string message;
        if (client->receive(message)) {
            broadcastMessage(message, client);
            addChatMessage(message);
        }
    }
}

void LobbyPage::handleClientUpdates() {
    std::string message;
    while (clientSocket->receive(message)) {
        addChatMessage(message);
    }
}

void LobbyPage::broadcastMessage(const std::string& message, std::shared_ptr<ClientSocket> sender) {
    if (serverSocket) {
        for (const auto& client : serverSocket->getClients()) {
            if (client != sender) {
                client->send(message);
            }
        }
    }
    //addChatMessage(message); // Display locally
}

void LobbyPage::broadcastUserList() {
    if (serverSocket) {
        std::string userList = "Players:\n";
        for (const auto& client : serverSocket->getClients()) {
            userList += client->getUsername() + "\n";
        }
        userList += localUsername + "\n";
        broadcastMessage(userList, nullptr);
    }
}

void LobbyPage::addPlayer(const std::string& playerName) {
    players.push_back(playerName);
    updatePlayerList();
}

void LobbyPage::updatePlayerList() {
    std::string playerText = "Players:\n";
    for (const auto& player : players) {
        playerText += player + "\n";
    }
    playerSidebar->copy_label(playerText.c_str());
}

void LobbyPage::addChatMessage(const std::string& message) {
    if (chatBuffer) {
        chatBuffer->append((message + "\n").c_str());
    }
}

void LobbyPage::send_button_callback(Fl_Widget* widget, void* userdata) {
    auto* lobbyPage = static_cast<LobbyPage*>(userdata);
    const char* text = lobbyPage->inputBox->value();
    if (text && text[0] != '\0') {
        std::string message = lobbyPage->localUsername + ": " + text;
        lobbyPage->addChatMessage(message);

        if (lobbyPage->serverSocket) {
            lobbyPage->broadcastMessage(message, nullptr);
        }
        else if (lobbyPage->clientSocket) {
            lobbyPage->broadcastMessage(message, nullptr);
            //lobbyPage->clientSocket->send(message);
        }

        lobbyPage->inputBox->value("");
    }
}
