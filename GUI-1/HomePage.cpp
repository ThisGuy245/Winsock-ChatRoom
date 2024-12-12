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

// Modified host button callback to allow the host to act as a client after hosting
void HomePage::host_button_callback(Fl_Widget* widget, void* userdata) {
    auto* homePage = static_cast<HomePage*>(userdata);
    if (!homePage) {
        fl_alert("Failed to retrieve HomePage instance!");
        return;
    }

    auto* mainWindow = homePage->m_mainWindow;
    if (!mainWindow) {
        fl_alert("Failed to retrieve MainWindow instance!");
        return;
    }

    const char* username = homePage->usernameInput->value();
    if (!username || strlen(username) == 0) {
        fl_alert("Username is required to host a server.");
        return;
    }

    try {
        // Create ServerSocket
        auto serverSocket = std::make_shared<ServerSocket>(8080);
        fl_alert("Hosting on port 8080. Waiting for clients...");

        // Set the username for the server
        serverSocket->setUsername(username);

        // Switch to LobbyPage
        mainWindow->switch_to_lobby(widget, mainWindow);
        auto* lobbyPage = mainWindow->getLobbyPage();
        if (!lobbyPage) {
            fl_alert("Failed to retrieve LobbyPage instance!");
            return;
        }

        // Pass the server socket and username to the lobby
        lobbyPage->initializeServer(serverSocket, username);

        // Start accepting connections in a separate thread
        std::thread([serverSocket, lobbyPage]() {
            try {
                while (true) {
                    auto newClient = serverSocket->accept();
                    if (newClient) {
                        serverSocket->addClient(newClient);
                        lobbyPage->updateConnectedUsers();  // Update user list when new client connects
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Avoid busy-waiting
                }
            }
            catch (const std::exception& e) {
                printf("Error accepting client: %s\n", e.what());
            }
            }).detach();

            // Ensure server socket is cleaned up when the application closes
            mainWindow->on_close([serverSocket]() mutable {
                if (serverSocket) {
                    serverSocket.reset();
                }
                });

            // Make the host also act as a client
            auto localClientSocket = std::make_shared<ClientSocket>();
            if (localClientSocket->ConnectToServer("127.0.0.1", 8080)) {
                localClientSocket->setUsername(username);
                lobbyPage->initializeClient(localClientSocket, username);
            }
    }
    catch (const std::exception& e) {
        fl_alert(("Error starting server: " + std::string(e.what())).c_str());
    }
}




void HomePage::join_button_callback(Fl_Widget* widget, void* userdata) {
    auto* homePage = static_cast<HomePage*>(userdata);
    if (!homePage) {
        fl_alert("Failed to retrieve HomePage instance!");
        return;
    }

    auto* mainWindow = homePage->m_mainWindow;
    if (!mainWindow) {
        fl_alert("Failed to retrieve MainWindow instance!");
        return;
    }

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

        // Create the client socket (local variable)
        auto localClientSocket = std::make_shared<ClientSocket>();
        printf("Attempting to connect to %s:8080\n", ip);

        if (localClientSocket->ConnectToServer(ip, 8080)) {
            fl_alert("Successfully connected to the server!");

            // Set the username
            localClientSocket->setUsername(username);

            // Switch to LobbyPage
            mainWindow->switch_to_lobby(widget, mainWindow);

            auto* lobbyPage = mainWindow->getLobbyPage();
            if (!lobbyPage) {
                fl_alert("Failed to retrieve LobbyPage instance!");
                return;
            }

            // Pass the client socket and username to the lobby
            lobbyPage->setClientSocket(localClientSocket, username);

                // Ensure client socket is cleaned up when the application closes
                mainWindow->on_close([localClientSocket]() mutable {
                    if (localClientSocket) {
                        localClientSocket->close();
                        localClientSocket.reset();
                    }
                    });

                // Assign to the global clientSocket for use in other parts of the program
                clientSocket = localClientSocket;
        }
        else {
            fl_alert("Failed to connect to the server. Please check the IP and try again.");
        }
    }
    catch (const std::exception& e) {
        fl_alert(("Connection error: " + std::string(e.what())).c_str());
    }
}
