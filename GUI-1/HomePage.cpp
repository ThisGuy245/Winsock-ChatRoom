#include <thread>
#include <FL/Fl_ask.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include "ServerSocket.h"
#include "ClientSocket.h"
#include "HomePage.h"
#include "MainWindow.h"
#include "LobbyPage.h"
#include <ws2tcpip.h>

HomePage::HomePage(int x, int y, int w, int h, MainWindow* mainWindow)
    : Fl_Group(x, y, w, h), m_mainWindow(mainWindow) { // Store the MainWindow pointer
    Fl_Box* title = new Fl_Box(x + w / 2 - 100, y + 50, 200, 30, "Home Page");
    title->labelsize(18);

    Fl_Button* hostButton = new Fl_Button(x + w / 4 - 50, y + h / 2 - 75, 100, 50, "Host");
    hostButton->callback(host_button_callback, m_mainWindow); // Pass MainWindow to callback

    Fl_Button* joinButton = new Fl_Button(x + w / 4 - 50, y + h / 2 + 25, 100, 50, "Join");
    joinButton->callback(join_button_callback, m_mainWindow); // Pass MainWindow to callback

    end();
}

HomePage::~HomePage() {
    // Destructor
}


// GLOBAL POINTERS
std::shared_ptr<ServerSocket> serverSocket;
std::shared_ptr<ClientSocket> clientSocket;

void HomePage::host_button_callback(Fl_Widget* widget, void* userdata) {
    auto* mainWindow = static_cast<MainWindow*>(userdata);
    if (!mainWindow) {
        fl_alert("Failed to retrieve MainWindow!");
        return;
    }

    try {
        // Create ServerSocket
        serverSocket = std::make_shared<ServerSocket>(8080);
        fl_alert("Hosting on port 8080. Waiting for clients...");

        // Switch to LobbyPage
        mainWindow->switch_to_lobby(widget, userdata);
        auto* lobbyPage = mainWindow->getLobbyPage();
        lobbyPage->setServerSocket(serverSocket);

        // Start accepting connections
        std::thread([lobbyPage]() {
            while (true) {
                try {
                    auto newClient = serverSocket->accept();
                    if (newClient) {
                        serverSocket->addClient(newClient);

                        // Update host's LobbyPage
                        Fl::lock();
                        int playerCount = serverSocket->getClients().size();
                        lobbyPage->addPlayer("Player " + std::to_string(playerCount));
                        lobbyPage->addChatMessage("Player " + std::to_string(playerCount) + " joined the lobby.");
                        Fl::unlock();
                        Fl::awake();

                        // Notify all clients (including host) of the new player
                        std::string message = "Player " + std::to_string(playerCount) + " joined.";
                        for (auto& client : serverSocket->getClients()) {
                            client->send(message); // Send the message to all connected clients
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

            mainWindow->on_close([]() {
                if (serverSocket) {
                    serverSocket.reset();
                }
                });
    }
    catch (const std::exception& e) {
        fl_alert(("Error starting server: " + std::string(e.what())).c_str());
    }
}

void HomePage::join_button_callback(Fl_Widget* widget, void* userdata) {
    auto* mainWindow = static_cast<MainWindow*>(userdata);

    const char* ip = fl_input("Enter host IP:", "127.0.0.1");
    if (!ip || strlen(ip) == 0) return;

    sockaddr_in sa;
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) != 1) {
        fl_alert("Invalid IP address. Please enter a valid IP.");
        return;
    }

    try {
        // Creation of the socket
        clientSocket = std::make_shared<ClientSocket>();
        printf("Attempting to connect to %s:8080\n", ip);

        if (clientSocket->ConnectToServer(ip, 8080)) {
            fl_alert("Successfully connected to the server!");

            // Set player ID (received from the server)
            int playerId = clientSocket->getPlayerId();  // You would set this after the connection is established.

            mainWindow->switch_to_lobby(widget, userdata);

            auto* lobbyPage = mainWindow->getLobbyPage();
            lobbyPage->setClientSocket(clientSocket);

            // Add player to lobby page
            lobbyPage->addPlayer("Player " + std::to_string(playerId));

            bool stopReceiving = false;

            std::thread([lobbyPage]() {
                std::string message;
                auto clientSocket = lobbyPage->getClientSocket();
                while (clientSocket->receive(message)) {
                    Fl::lock();
                    lobbyPage->addChatMessage(message); // Display message from server
                    Fl::unlock();
                    Fl::awake();
                }
                }).detach();

                mainWindow->on_close([&stopReceiving]() {
                    stopReceiving = true;
                    if (clientSocket) clientSocket->close();
                    });
        }
        else {
            fl_alert("Failed to connect to the server. Please check the IP and try again.");
        }
    }
    catch (const std::exception& e) {
        fl_alert(("Connection error: " + std::string(e.what())).c_str());
    }
}

