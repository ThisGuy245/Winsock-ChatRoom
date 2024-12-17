#include "LobbyPage.hpp"
#include <FL/Fl_Ask.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Scroll.H>
#include <FL/fl_draw.H>
#include "HomePage.hpp"
#include "MainWindow.h"
#include "SettingsWindow.hpp"
#include "AboutWindow.h"

LobbyPage::LobbyPage(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H), client(nullptr), server(nullptr) {
    begin();

    // Create the Menu Bar at the top
    menuBar = new Fl_Menu_Bar(0, 0, W, 30);
    menuBar->add("Server/Disconnect", FL_CTRL + 'd', menuCallback, (void*)this);
    menuBar->add("Server/Quit", FL_CTRL + 'q', menuCallback, (void*)this);
    menuBar->add("Settings/Preferences", FL_CTRL + 'p', menuCallback, (void*)this);
    menuBar->add("About/Application Info", FL_CTRL + 'a', menuCallback, (void*)this);


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

    // Configure messageInput to handle the Enter key
    messageInput->when(FL_WHEN_ENTER_KEY_ALWAYS);
    messageInput->callback([](Fl_Widget* widget, void* userdata) {
        LobbyPage* page = static_cast<LobbyPage*>(userdata);
        if (page) {
            Fl_Input* input = static_cast<Fl_Input*>(widget);
            const std::string message = input->value();
            if (!message.empty()) {
                page->sendMessage(message);
                input->value("");  // Clear the input field after sending
            }
        }
        }, this);


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
    this->username = username;  // Set the username for this session
    server = new ServerSocket(12345);

    // Announce the server creation
    chatBuffer->append("This is a buffer line\n");
    chatBuffer->append("Server has been created\n");

    client = new ClientSocket(ip, 12345, username);  // Client also joins the server
    // Announce the client joining
    chatBuffer->append((username + " has joined the server\n").c_str());
}

void LobbyPage::joinServer(const std::string& ip, const std::string& username) {
    this->username = username;  // Set the username for this session
    client = new ClientSocket(ip, 12345, username);  // Client joins the server
    chatBuffer->append("This is a buffer line\n");
    // Announce the client joining
    chatBuffer->append((username + " has joined the server\n").c_str());
}

void LobbyPage::clientLeft(const std::string& username) {
    chatBuffer->append((username + " has left the server\n").c_str());
}


void LobbyPage::sendMessage(const std::string& message) {
    if (client) {
        client->send(message);  // Send the message, not the username
        chatBuffer->append((username + ": " + message + "\n").c_str());  // Send the username as part of the message
    }
}

void LobbyPage::receiveMessages() {
    std::string message;
    if (client && client->receive(message)) {
        chatBuffer->append((message + "\n").c_str());  // Message already includes username
    }

    if (server) {
        server->handleClientConnections();  // Handle incoming server-client connections
    }
}

// This is called every frame to keep the chat updated
void LobbyPage::Update() {
    receiveMessages();
}



void LobbyPage::menuCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* page = static_cast<LobbyPage*>(userdata);
    Fl_Menu_Bar* menu = static_cast<Fl_Menu_Bar*>(widget);

    MainWindow* mainWindow = dynamic_cast<MainWindow*>(page->parent());

    // Get the index of the selected menu item
    int menu_index = menu->value();  // This is the index, not the menu item value

    switch (menu_index) {
    case 1: {  // Server/Disconnect
        if (page->client || page->server) {
            delete page->client;
            delete page->server;
            page->client = nullptr;
            page->server = nullptr;
            fl_message("Disconnected from the server.");
        }

        if (mainWindow) {
            mainWindow->switch_to_home(nullptr, mainWindow);
        }
        break;
    }

    case 2: {  // Server/Quit
        exit(0);
        break;
    }

    case 5: {  // Settings
        SettingsWindow* settings = new SettingsWindow(400, 250, "Settings");
        settings->show();
        break;
    }

    case 8: {  // About
        AboutWindow* about = new AboutWindow(400, 250, "About");
        about->show();
        break;
    }

    default: {
        int menu_index = menu->value();  // This is the index of the selected menu item
        fl_message("Menu item selected with index: %d", menu_index);  // Print the selected index

        break;
    }
    }
}



void LobbyPage::applyStyles() {
    // Apply styles for dark mode readiness
    menuBar->color(FL_DARK3);      // Menu bar background
    menuBar->textcolor(FL_WHITE); // Menu bar text color

    chatDisplay->textfont(FL_HELVETICA); // Clean font for chat
    chatDisplay->textsize(14);          // Slightly larger text
    chatDisplay->color(FL_WHITE);
    chatDisplay->textcolor(FL_BLACK);

    // Future dark mode adjustments
    //chatDisplay->color(FL_BLACK); // Uncomment for dark background
    //chatDisplay->textcolor(FL_WHITE); // Uncomment for white text
}


