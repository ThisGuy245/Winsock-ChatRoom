// HomePage.cpp
#include "HomePage.hpp"
#include "MainWindow.h"
#include "LobbyPage.hpp"
#include <FL/fl_ask.H>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

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

HomePage::~HomePage() {
    delete usernameInput;
    delete ipInput;
    delete hostButton;
    delete joinButton;
}

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
