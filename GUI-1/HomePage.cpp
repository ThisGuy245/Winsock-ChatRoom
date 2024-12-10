#include "HomePage.hpp"
#include "LobbyPage.hpp"

#include "MainWindow.h"
#include "ServerSocket.h"
#include "ClientSocket.h"
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <iostream>
#include <thread>
#include <ws2tcpip.h>

HomePage::HomePage(int x, int y, int w, int h, MainWindow* mainWindow)
    : Fl_Group(x, y, w, h), m_mainWindow(mainWindow), lobbyPage(nullptr) {

    // Title
    Fl_Box* title = new Fl_Box(x + w / 2 - 100, y + 50, 200, 30, "Home Page");
    title->labelsize(18);

    // Username input
    Fl_Box* usernameLabel = new Fl_Box(x + w / 2 - 100, y + 100, 200, 30, "Enter Username:");
    usernameLabel->labelsize(14);

    usernameInput = new Fl_Input(x + w / 2 - 100, y + 130, 200, 30);
    usernameInput->callback(username_input_callback, this);  // Set callback for username input

    // Login button
    loginButton = new Fl_Button(x + w / 2 - 50, y + 170, 100, 50, "Login");
    loginButton->callback(login_button_callback, this);  // Set callback for login button

    // Host button (initially disabled)
    hostButton = new Fl_Button(x + w / 4 - 50, y + h / 2 - 75, 100, 50, "Host");
    hostButton->callback(host_button_callback, this);
    hostButton->deactivate();  // Initially disabled

    // Join button (initially disabled)
    joinButton = new Fl_Button(x + w / 4 - 50, y + h / 2 + 25, 100, 50, "Join");
    joinButton->callback(join_button_callback, this);
    joinButton->deactivate();  // Initially disabled

    end();
}

HomePage::~HomePage() {
    // Destructor
}

// Callback when the username input changes
void HomePage::username_input_callback(Fl_Widget* widget, void* userdata) {
    HomePage* homePage = static_cast<HomePage*>(userdata);
    const char* username = homePage->usernameInput->value();

    // Disable buttons if the username is empty
    if (username && strlen(username) > 0) {
        homePage->loginButton->activate();  // Enable login button when username is valid
    }
    else {
        homePage->loginButton->deactivate();  // Disable login button if username is empty
    }
}

// Callback for the login button
void HomePage::login_button_callback(Fl_Widget* widget, void* userdata) {
    HomePage* homePage = static_cast<HomePage*>(userdata);
    const char* username = homePage->usernameInput->value();

    // Check if the username is valid (non-empty)
    if (username && strlen(username) > 0) {
        fl_alert("Login successful!");

        // Enable the Host and Join buttons after a valid login
        homePage->hostButton->activate();
        homePage->joinButton->activate();
    }
    else {
        fl_alert("Please enter a valid username.");
    }
}

// GLOBAL POINTERS
std::shared_ptr<ServerSocket> serverSocket;
std::shared_ptr<ClientSocket> clientSocket;

void HomePage::host_button_callback(Fl_Widget* widget, void* userdata) {
    auto* homePage = static_cast<HomePage*>(userdata);
    auto* mainWindow = homePage->m_mainWindow;
    if (!mainWindow) {
        fl_alert("Failed to retrieve MainWindow!");
        return;
    }

    const char* username = homePage->usernameInput->value();
    if (!username || strlen(username) == 0) {
        fl_alert("Username is required to host a server.");
        return;
    }

    try {
        // Create ServerSocket
        serverSocket = std::make_shared<ServerSocket>(8080);
        fl_alert("Hosting on port 8080. Waiting for clients...");

        // Set the username
        serverSocket->setUsername(username);

        // Switch to LobbyPage
        mainWindow->switch_to_lobby(widget, userdata);
        auto* lobbyPage = mainWindow->getLobbyPage();
        lobbyPage->setServerSocket(serverSocket, username); // Pass the server socket

        // Start accepting connections
        std::thread([lobbyPage]() {
            while (true) {
                try {
                    auto newClient = serverSocket->accept();
                    if (newClient) {
                        serverSocket->addClient(newClient);
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
    auto* homePage = static_cast<HomePage*>(userdata);
    auto* mainWindow = homePage->m_mainWindow;

    const char* ip = fl_input("Enter host IP:", "127.0.0.1");
    if (!ip || strlen(ip) == 0) return;

    sockaddr_in sa;
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) != 1) {
        fl_alert("Invalid IP address. Please enter a valid IP.");
        return;
    }

    try {
        // Retrieve username from input field
        const char* username = homePage->usernameInput->value();
        if (!username || strlen(username) == 0) {
            fl_alert("Username is required to join a server.");
            return;
        }

        // Create the client socket
        clientSocket = std::make_shared<ClientSocket>();
        printf("Attempting to connect to %s:8080\n", ip);

        if (clientSocket->ConnectToServer(ip, 8080)) {
            fl_alert("Successfully connected to the server!");

            // Set the username
            clientSocket->setUsername(username);

            // Switch to LobbyPage
            mainWindow->switch_to_lobby(widget, userdata);

            auto* lobbyPage = mainWindow->getLobbyPage();
            lobbyPage->setClientSocket(clientSocket, username); // Pass the client socket

            bool stopReceiving = false;
            std::thread([lobbyPage]() {
                std::string message;
                auto clientSocket = lobbyPage->getClientSocket();
                while (clientSocket->receive(message)) {
                    Fl::lock();
                    lobbyPage->addChatMessage(message);
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
