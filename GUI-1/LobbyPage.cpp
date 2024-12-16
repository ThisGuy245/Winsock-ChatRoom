#include "LobbyPage.hpp"
#include <FL/Fl_Ask.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Scroll.H>
#include "HomePage.hpp" // Include HomePage header to navigate back to HomePage

LobbyPage::LobbyPage(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H), client(nullptr), server(nullptr) {
    begin();

    // Create the Menu Bar at the top
    menuBar = new Fl_Menu_Bar(0, 0, W, 30);
    menuBar->add("Server/Host", FL_CTRL + 'h', menuCallback, (void*)this);
    menuBar->add("Server/Join", FL_CTRL + 'j', menuCallback, (void*)this);
    menuBar->add("Settings/Preferences", FL_CTRL + 'p', menuCallback, (void*)this);
    menuBar->add("About", FL_CTRL + 'a', menuCallback, (void*)this);

    // Create a scrollable area for chat history and message input
    scrollArea = new Fl_Scroll(0, 30, W, H - 30);
    scrollArea->type(FL_VERTICAL);

    // Display area for chat history with a text buffer
    chatDisplay = new Fl_Text_Display(10, 10, W - 20, H - 120);
    chatBuffer = new Fl_Text_Buffer();
    chatDisplay->buffer(chatBuffer);
    chatDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);


    // Message input field
    messageInput = new Fl_Input(W / 2 - 150, H - 90, 300, 30, "Message:");

    // Send button
    sendButton = new Fl_Button(W / 2 - 50, H - 50, 100, 30, "Send");
    sendButton->callback([](Fl_Widget* widget, void* userdata) {
        LobbyPage* page = static_cast<LobbyPage*>(userdata);
        if (page) {
            const std::string message = page->messageInput->value();
            page->sendMessage(message);
            page->messageInput->value("");  // Clear input after sending
        }
        }, this);

    end();
    resizable(scrollArea);
}

LobbyPage::~LobbyPage() {
    delete client;
    delete server;
}

void LobbyPage::hostServer(const std::string& ip, const std::string& username) {
    server = new ServerSocket(12345);
    client = new ClientSocket(ip, 12345);
}

void LobbyPage::joinServer(const std::string& ip, const std::string& username) {
    client = new ClientSocket(ip, 12345);
}

void LobbyPage::sendMessage(const std::string& message) {
    if (client) {
        client->send(username, message);
        //chatBuffer->append(("You: " + message + "\n").c_str());
    }
}

void LobbyPage::receiveMessages() {
    std::string message;
    if (client && client->receive(username, message)) {
        // Display the received message with the username
        chatBuffer->append((username + ": " + message + "\n").c_str());
    }
    if (server) {
        server->handleClientConnections();
    }
}


// This is called every frame to keep the chat updated
void LobbyPage::Update() {
    receiveMessages();
}

void LobbyPage::menuCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* page = static_cast<LobbyPage*>(userdata);
    Fl_Menu_Bar* menu = static_cast<Fl_Menu_Bar*>(widget);

    switch (menu->value()) {
    case 1: // Host server
        page->hostServer("127.0.0.1", "User1");
        break;
    case 2: // Join server
        page->joinServer("127.0.0.1", "User2");
        break;
    case 3: // Settings
        fl_message("Settings preferences window would open here.");
        break;
    case 4: // About
        fl_message("About this app: Simple Chat App.");
        break;
    case 5: // Server - Disconnect and go back to HomePage
        if (page->client) {
            // Disconnect the client and server
            page->server = nullptr;
            page->client = nullptr;
        }
        break;
    }
}
