#include "HomePage.hpp"
#include "MainWindow.h"
#include "LobbyPage.hpp"
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

/**
 * @brief Constructor for HomePage class.
 * Initializes the user interface for setting a username and joining or hosting a server.
 * @param X The x-coordinate of the window.
 * @param Y The y-coordinate of the window.
 * @param W The width of the window.
 * @param H The height of the window.
 * @param parent A pointer to the parent MainWindow instance.
 */
HomePage::HomePage(int X, int Y, int W, int H, MainWindow* parent)
    : Fl_Group(X, Y, W, H), mainWindow(parent) {

    usernameInput = new Fl_Input(W / 2 - 100, H / 2 - 60, 200, 30, "Username:");
    usernameInput->align(FL_ALIGN_TOP);

    ipInput = new Fl_Input(W / 2 - 100, H / 2 - 10, 200, 30, "Server IP:");
    ipInput->align(FL_ALIGN_TOP);
    ipInput->value(getLocalIPAddress().c_str()); // Default to local IP

    hostButton = new Fl_Button(W / 2 - 100, H / 2 + 40, 90, 30, "Host");
    hostButton->callback(hostButtonCallback, this);

    joinButton = new Fl_Button(W / 2 + 10, H / 2 + 40, 90, 30, "Join");
    joinButton->callback(joinButtonCallback, this);

    end();  // Finalize the layout
}

/**
 * @brief Destructor for HomePage class. Cleans up allocated UI components.
 */
HomePage::~HomePage() {
    delete usernameInput;
    delete ipInput;
    delete hostButton;
    delete joinButton;
}

/**
 * @brief Callback function for the "Host" button.
 * Validates user input and attempts to host a server.
 * @param widget The widget that triggered the callback.
 * @param userdata A pointer to the HomePage instance.
 */
void HomePage::hostButtonCallback(Fl_Widget* widget, void* userdata) {
    HomePage* homePage = static_cast<HomePage*>(userdata);
    if (!homePage) return;

    const char* username = homePage->usernameInput->value();
    if (!username || strlen(username) == 0) {
        fl_alert("Please enter a username.");
        return;
    }

    const char* ip = homePage->ipInput->value();
    // Attempt to host a server
    try {
        LobbyPage* lobbyPage = homePage->mainWindow->getLobbyPage();
        lobbyPage->setUsername(username); // Set the username
        lobbyPage->hostServer(ip, username);
        homePage->mainWindow->switch_to_lobby(widget, homePage->mainWindow);
    }
    catch (const std::runtime_error& e) {
        fl_alert("Failed to host server: %s", e.what());
    }
}

/**
 * @brief Callback function for the "Join" button.
 * Validates user input and attempts to join an existing server.
 * @param widget The widget that triggered the callback.
 * @param userdata A pointer to the HomePage instance.
 */
void HomePage::joinButtonCallback(Fl_Widget* widget, void* userdata) {
    HomePage* homePage = static_cast<HomePage*>(userdata);
    if (!homePage) return;

    const char* username = homePage->usernameInput->value();
    const char* ip = homePage->ipInput->value();
    if (!username || strlen(username) == 0) {
        fl_alert("Please enter a username.");
        return;
    }

    if (!ip || strlen(ip) == 0) {
        fl_alert("Please enter a server IP.");
        return;
    }

    // Attempt to join the server
    try {
        LobbyPage* lobbyPage = homePage->mainWindow->getLobbyPage();
        lobbyPage->setUsername(username); // Set the username
        lobbyPage->joinServer(ip, username);
        homePage->mainWindow->switch_to_lobby(widget, homePage->mainWindow);
    }
    catch (const std::runtime_error& e) {
        fl_alert("Failed to join server: %s", e.what());
    }
}

/**
 * @brief Applies dark or light theme to all HomePage widgets.
 * @param isDarkMode True for dark mode, false for light mode.
 */
void HomePage::applyTheme(bool isDarkMode) {
    Fl_Color bgColor = isDarkMode ? fl_rgb_color(45, 45, 45) : fl_rgb_color(255, 255, 255);
    Fl_Color textColor = isDarkMode ? FL_WHITE : FL_BLACK;
    Fl_Color buttonColor = isDarkMode ? fl_rgb_color(70, 70, 70) : FL_LIGHT2;
    
    // Style input fields
    if (usernameInput) {
        usernameInput->color(bgColor);
        usernameInput->textcolor(textColor);
        usernameInput->redraw();
    }
    
    if (ipInput) {
        ipInput->color(bgColor);
        ipInput->textcolor(textColor);
        ipInput->redraw();
    }
    
    // Style buttons
    if (hostButton) {
        hostButton->color(buttonColor);
        hostButton->labelcolor(textColor);
        hostButton->redraw();
    }
    
    if (joinButton) {
        joinButton->color(buttonColor);
        joinButton->labelcolor(textColor);
        joinButton->redraw();
    }
    
    redraw();
}

/**
 * @brief Retrieves the local IP address of the machine.
 * Uses `gethostname` and `getaddrinfo` to resolve the local IP.
 * @return A string containing the local IP address.
 * @throws std::runtime_error if there is an issue retrieving the local IP.
 */
std::string HomePage::getLocalIPAddress() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to get local hostname.");
    }

    addrinfo hints = {}, * result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
        throw std::runtime_error("Failed to resolve local address.");
    }

    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ipBuffer, sizeof(ipBuffer));

    freeaddrinfo(result);
    return std::string(ipBuffer);
}
