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
    // Initialize chat data
    chatData = std::make_shared<ChatData>();
    chatData->load("chat_history.xml");

    // Create UI elements
    Fl_Box* navBar = new Fl_Box(x, y, w, 50, "");
    navBar->box(FL_FLAT_BOX);
    navBar->color(FL_BLUE);

    Fl_Box* titleBox = new Fl_Box(x + 10, y + 10, 200, 30, "Lobby Page");
    titleBox->labelsize(18);
    titleBox->align(FL_ALIGN_CENTER);

    connectionStatusBox = new Fl_Box(x + w - 150, y + 10, 140, 30, "Connected");
    connectionStatusBox->labelsize(14);
    connectionStatusBox->align(FL_ALIGN_CENTER);

    // Player sidebar
    playerSidebar = new Fl_Box(x, y + 50, 200, h - 100, "");
    playerSidebar->box(FL_UP_BOX);
    playerSidebar->label("Players\n");
    playerSidebar->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    // Chat display
    chatBuffer = new Fl_Text_Buffer();
    chatDisplay = new Fl_Text_Display(x + 220, y + 60, w - 240, h - 150);
    chatDisplay->box(FL_BORDER_BOX);
    chatDisplay->color(FL_WHITE);
    chatDisplay->buffer(chatBuffer);

    // Input box and send button
    inputBox = new Fl_Input(x + 220, y + h - 90, w - 320, 30, "Message:");
    inputBox->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);

    Fl_Button* sendButton = new Fl_Button(x + w - 100, y + h - 90, 80, 30, "Send");
    sendButton->callback(send_button_callback, (void*)this);

    // Display chat history
    for (const auto& message : chatData->getMessages()) {
        addChatMessage(message.user + ": " + message.content);
    }

    end(); // End the group
}

void LobbyPage::setServerSocket(std::shared_ptr<ServerSocket> server, const std::string& username) {
    serverSocket = server;
    localUsername = username;
    addPlayer(localUsername);
    std::string joinMessage = localUsername + " joined the chat.";
    broadcastMessage(joinMessage, nullptr);  // Broadcast as a regular client message, no specific sender
    broadcastUserList();
}

void LobbyPage::setClientSocket(std::shared_ptr<ClientSocket> client, const std::string& username) {
    clientSocket = client;
    localUsername = username;

    // Notify host of the username
    clientSocket->send(localUsername + " joined the chat.");

    // Add this client to the player list
    addPlayer(localUsername);

    // Display chat history
    for (const auto& message : chatData->getMessages()) {
        addChatMessage(message.user + ": " + message.content);
    }

    // Handle incoming messages via Update()
}

void LobbyPage::Update() {
    // Server-side logic
    if (serverSocket) {
        // Accept new clients
        auto newClient = serverSocket->accept();
        if (newClient) {
            serverSocket->addClient(newClient);
            addPlayer(newClient->getUsername()); // Sync username
            broadcastUserList();
        }

        // Receive and broadcast messages
        for (auto& client : serverSocket->getClients()) {
            std::string message;
            if (client->receive(message)) {
                chatData->addMessage(client->getUsername(), message); // Include usernames
                broadcastMessage(message, client);
                addChatMessage(client->getUsername() + ": " + message);
            }
        }
    }
    // Client-side logic
    if (clientSocket) {
        std::string message;
        while (clientSocket->receive(message)) {
            addChatMessage(message);
        }
    }
}

void LobbyPage::broadcastMessage(const std::string& message, std::shared_ptr<ClientSocket> sender) {
    if (serverSocket) {
        for (const auto& client : serverSocket->getClients()) {
            // Only send messages to clients if they are not the sender.
            if (!sender || client != sender) {
                client->send(message);
            }
        }
    }
}

void LobbyPage::broadcastUserList() {
    if (serverSocket) {
        std::string userList = "Users in the lobby:\n";
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
    std::string playerText = "Players\n";
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
    const char* message = lobbyPage->inputBox->value();

    if (message && message[0] != '\0') {
        std::string formattedMessage = lobbyPage->localUsername + ": " + message;
        lobbyPage->chatData->addMessage(lobbyPage->localUsername, message);
        lobbyPage->addChatMessage(formattedMessage);
        if (lobbyPage->clientSocket) {
            lobbyPage->clientSocket->send(formattedMessage);
        }
        else if (lobbyPage->serverSocket) {
            lobbyPage->broadcastMessage(formattedMessage, nullptr); // Message from host as a client
        }
        lobbyPage->inputBox->value(""); // Clear input box
    }
}
