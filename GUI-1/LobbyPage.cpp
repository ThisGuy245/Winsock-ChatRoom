#include "LobbyPage.h"
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <algorithm>
#include <thread>
#include <string>
#include <sstream>

void LobbyPage::setServerSocket(std::shared_ptr<ServerSocket> server) {
    serverSocket = server;

    // Add the host to the player list
    Fl::lock();
    addPlayer("Player 1");
    addChatMessage("Player 1 joined the lobby.");
    Fl::unlock();
    Fl::awake();

    // Start a thread to handle client connections
    std::thread([this]() {
        while (true) {
            try {
                auto newClient = serverSocket->accept();
                if (newClient) {
                    serverSocket->addClient(newClient);

                    // Add the client and notify other clients
                    Fl::lock();
                    int playerCount = serverSocket->getClients().size() + 1; // +1 to account for the host
                    std::string playerName = "Player " + std::to_string(playerCount);
                    addPlayer(playerName);
                    addChatMessage(playerName + " joined the lobby.");
                    Fl::unlock();
                    Fl::awake();

                    // Broadcast player joined message to all clients
                    std::string joinMessage = playerName + " joined the lobby.";
                    for (const auto& client : serverSocket->getClients()) {
                        client->send(joinMessage);
                    }
                }
            }
            catch (const std::exception& e) {
                printf("Error accepting client: %s\n", e.what());
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Avoid busy-waiting
        }
        }).detach();

        // Start a thread to handle messages from clients
        // Server-side thread (handling connections and broadcasting messages)
        std::thread([this]() {
            while (true) {
                try {
                    for (auto& client : serverSocket->getClients()) {
                        std::string message;
                        if (client->receive(message)) {
                            // Broadcast the message to all other clients (not the same client)
                            Fl::lock();
                            addChatMessage(message);  // Show message on the host
                            Fl::unlock();
                            Fl::awake();

                            for (const auto& otherClient : serverSocket->getClients()) {
                                if (otherClient != client) {
                                    otherClient->send(message);  // Avoid resending the message to the sender
                                }
                            }
                        }
                    }
                }
                catch (const std::exception& e) {
                    printf("Error receiving message: %s\n", e.what());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Avoid busy-waiting
            }
            }).detach();

}



void LobbyPage::setClientSocket(std::shared_ptr<ClientSocket> client) {
    clientSocket = client;

    // Add the client (Player 2) to the player list
    Fl::lock();
    addPlayer("Player 2");
    addChatMessage("Player 2 joined the lobby.");
    Fl::unlock();
    Fl::awake();

    // Start a thread to listen for server messages
    std::thread([this]() {
        std::string message;
        while (clientSocket->receive(message)) {
            Fl::lock();
            //addChatMessage(message); // Display received message
            Fl::unlock();
            Fl::awake();
        }
        }).detach();
}


LobbyPage::LobbyPage(int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), chatDisplay(nullptr), chatBuffer(nullptr), inputBox(nullptr) {
    // ** TOP NAVIGATION BAR **
    Fl_Box* navBar = new Fl_Box(x, y, w, 50, "");
    navBar->box(FL_FLAT_BOX);
    navBar->color(FL_BLUE);

    Fl_Box* titleBox = new Fl_Box(x + 10, y + 10, 200, 30, "Lobby Page");
    titleBox->labelsize(18);
    titleBox->align(FL_ALIGN_CENTER);

    Fl_Box* connectionStatusBox = new Fl_Box(x + w - 150, y + 10, 140, 30, "Connected");
    connectionStatusBox->labelsize(14);
    connectionStatusBox->align(FL_ALIGN_CENTER);

    // ** PLAYER SIDEBAR **
    playerSidebar = new Fl_Box(x, y + 50, 200, h - 100, "");
    playerSidebar->box(FL_UP_BOX);
    playerSidebar->label("Players\n");
    playerSidebar->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    // ** CHAT AREA **
    chatBuffer = new Fl_Text_Buffer();
    chatDisplay = new Fl_Text_Display(x + 220, y + 60, w - 240, h - 150);
    chatDisplay->box(FL_BORDER_BOX);
    chatDisplay->color(FL_WHITE);
    chatDisplay->buffer(chatBuffer);

    inputBox = new Fl_Input(x + 220, y + h - 90, w - 320, 30, "Message:");
    inputBox->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);

    Fl_Button* sendButton = new Fl_Button(x + w - 100, y + h - 90, 80, 30, "Send");
    sendButton->callback(send_button_callback, (void*)this);

    end(); // End the group
}

LobbyPage::~LobbyPage() {
    delete chatDisplay;
}

void LobbyPage::send_button_callback(Fl_Widget* widget, void* userdata) {
    auto* lobbyPage = static_cast<LobbyPage*>(userdata);
    const char* message = lobbyPage->inputBox->value();

    if (message && message[0] != '\0') {
        std::string formattedMessage;

        if (lobbyPage->serverSocket) {
            // Host sends message
            formattedMessage = "Player 1: " + std::string(message);
            lobbyPage->addChatMessage(formattedMessage);

            // Broadcast to all clients
            for (const auto& client : lobbyPage->serverSocket->getClients()) {
                client->send(formattedMessage);
            }
        }
        else if (lobbyPage->clientSocket) {
            // Client sends message to server
            formattedMessage = "Player 2: " + std::string(message);
            lobbyPage->clientSocket->send(formattedMessage);
        }

        // Clear input box
        lobbyPage->inputBox->value("");
    }
}


std::shared_ptr<ClientSocket> LobbyPage::getClientSocket() const {
    return clientSocket;
}

// Add a message to the chat
void LobbyPage::addChatMessage(const std::string& message) {
    if (chatBuffer) {
        chatBuffer->append((message + "\n").c_str());
    }
}

// Add a player to the list
void LobbyPage::addPlayer(const std::string& playerName) {
    players.push_back(playerName);
    updatePlayerList();
}

// Remove a player from the list
void LobbyPage::removePlayer(const std::string& playerName) {
    players.erase(std::remove(players.begin(), players.end(), playerName), players.end());
    updatePlayerList();
}

// Update the player list display
void LobbyPage::updatePlayerList() {
    std::string playerText = "Players\n";
    for (const auto& player : players) {
        playerText += player + "\n";
    }
    playerSidebar->copy_label(playerText.c_str());
}
