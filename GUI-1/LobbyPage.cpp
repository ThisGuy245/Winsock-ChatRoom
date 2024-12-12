#include "LobbyPage.hpp"
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <sstream>
#include <algorithm>
#include <iostream>

LobbyPage::LobbyPage(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), chatDisplay(nullptr), chatBuffer(nullptr), userListDisplay(nullptr), userListBuffer(nullptr), inputBox(nullptr) {
    // Sidebar for user list
    userListBuffer = new Fl_Text_Buffer();
    userListDisplay = new Fl_Text_Display(x, y, 200, h);
    userListDisplay->box(FL_UP_BOX);
    userListDisplay->buffer(userListBuffer);
    userListDisplay->labelsize(14);
    userListDisplay->textsize(12);

    // Chat display
    chatBuffer = new Fl_Text_Buffer();
    chatDisplay = new Fl_Text_Display(x + 210, y, w - 220, h - 60);
    chatDisplay->box(FL_BORDER_BOX);
    chatDisplay->buffer(chatBuffer);
    chatDisplay->textsize(12);

    // Input box
    inputBox = new Fl_Input(x + 210, y + h - 50, w - 300, 30, "Message:");

    // Send button
    Fl_Button* sendButton = new Fl_Button(x + w - 80, y + h - 50, 70, 30, "Send");
    sendButton->callback(sendMessageCallback, this);

    end();
    applyStyling();
}

void LobbyPage::applyStyling() {
    userListDisplay->color(FL_LIGHT3);
    chatDisplay->color(FL_WHITE);
    inputBox->color(FL_WHITE);
}

void LobbyPage::Update() {
    if (serverSocket) {
        processServerUpdates();
    }
    if (clientSocket) {
        processClientUpdates();
    }
}

void LobbyPage::processServerUpdates() {
    auto newClient = serverSocket->accept();
    if (newClient) {
        serverSocket->addClient(newClient);
        updateConnectedUsers();
    }

    for (auto& client : serverSocket->getClients()) {
        std::string message;
        if (client->receive(message)) {
            sendMessageToAll(message, client);
        }
    }
}

void LobbyPage::processClientUpdates() {
    std::string message;
    while (clientSocket->receive(message)) {
        if (message.compare(0, 8, "Players:") == 0) {
            refreshUserList(message);
        }
        else {
            appendChatMessage(message);
        }
    }
}

void LobbyPage::sendMessageToAll(const std::string& message, std::shared_ptr<ClientSocket> sender) {
    if (serverSocket) {
        for (const auto& client : serverSocket->getClients()) {
            if (client != sender) {
                client->send(message);
            }
        }
        appendChatMessage(message);
    }
    if (clientSocket) {
        clientSocket->send(message);
    }
}

void LobbyPage::initializeServer(std::shared_ptr<ServerSocket> server, const std::string& username) {
    serverSocket = server;
    localUsername = username;
    players.push_back(username);
    updateConnectedUsers();
    sendMessageToAll(username + " has joined the chat.");
}

void LobbyPage::initializeClient(std::shared_ptr<ClientSocket> client, const std::string& username) {
    clientSocket = client;
    localUsername = username;
    appendChatMessage("Connected to the server as " + username + ".");
    clientSocket->send(username + " has joined the chat.");
    sendUserList();  // Send the updated user list to the server (and other clients)
}

void LobbyPage::updateConnectedUsers() {
    std::string userList = "Players:\n";
    for (const auto& client : serverSocket->getClients()) {
        userList += client->getUsername() + "\n";
    }
    userList += localUsername + "\n";
    sendMessageToAll(userList);
    userListBuffer->text(userList.c_str());  // Update the user list in the UI
}

void LobbyPage::sendUserList() {
    std::string userList = "Players:\n";
    if (serverSocket) {
        for (const auto& client : serverSocket->getClients()) {
            userList += client->getUsername() + "\n";
        }
    }
    userList += localUsername + "\n";
    if (clientSocket) {
        clientSocket->send(userList);
    }
}

// Update user list based on messages from other clients
void LobbyPage::refreshUserList(const std::string& message) {
    players.clear();
    std::istringstream stream(message.substr(9)); // Skip "Players:\n"
    std::string player;
    while (std::getline(stream, player)) {
        if (!player.empty()) {
            players.push_back(player);
        }
    }
    userListBuffer->text(message.substr(9).c_str());  // Update the side panel
}


// Update user list based on messages from other clients
void LobbyPage::refreshUserList(const std::string& message) {
    players.clear();
    std::istringstream stream(message.substr(9)); // Skip "Players:\n"
    std::string player;
    while (std::getline(stream, player)) {
        if (!player.empty()) {
            players.push_back(player);
        }
    }
    userListBuffer->text(message.substr(9).c_str());  // Update the side panel
}


void LobbyPage::appendChatMessage(const std::string& message) {
    if (chatBuffer) {
        chatBuffer->append((message + "\n").c_str());
    }
}

void LobbyPage::sendMessageCallback(Fl_Widget* widget, void* userdata) {
    auto* lobbyPage = static_cast<LobbyPage*>(userdata);
    const char* text = lobbyPage->inputBox->value();
    if (text && text[0] != '\0') {
        std::string message = lobbyPage->localUsername + ": " + text;
        if (lobbyPage->serverSocket) {
            lobbyPage->sendMessageToAll(message);
        }
        else if (lobbyPage->clientSocket) {
            lobbyPage->clientSocket->send(message);
        }
        lobbyPage->inputBox->value("");
    }
}
