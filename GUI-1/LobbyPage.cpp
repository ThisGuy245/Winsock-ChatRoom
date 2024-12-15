#include "LobbyPage.hpp"
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Ask.H>
#include <FL/Fl.H>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// Constructor to initialize the LobbyPage
LobbyPage::LobbyPage(int X, int Y, int W, int H)
    : Fl_Window(X, Y, W, H, "Lobby") {

    // Create the menu bar
    menuBar = new Fl_Menu_Bar(0, 0, W, 30);
    menuBar->add("Server/Disconnect", 0, menuCallback, this);
    menuBar->add("Server/Exit", 0, menuCallback, this);
    menuBar->add("Settings/Preferences", 0, menuCallback, this);
    menuBar->add("About/About App", 0, menuCallback, this);

    // Create the message display area
    messageDisplay = new Fl_Text_Display(10, 40, W - 220, H - 140);
    messageBuffer = new Fl_Text_Buffer();
    messageDisplay->buffer(messageBuffer);
    messageDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    messageDisplay->box(FL_DOWN_BOX);
    messageDisplay->color(FL_WHITE);
    messageDisplay->textfont(FL_COURIER);
    messageDisplay->textsize(14);

    // Create the user list browser
    userListBrowser = new Fl_Hold_Browser(W - 200, 40, 190, H - 140, "Users:");
    userListBrowser->box(FL_DOWN_BOX);

    // Create the input field for typing messages
    messageInput = new Fl_Input(10, H - 90, W - 120, 30, "Message:");
    messageInput->box(FL_DOWN_BOX);

    // Create the send button
    sendButton = new Fl_Button(W - 100, H - 90, 90, 30, "Send");
    sendButton->callback(sendButtonCallback, this);

    end(); // Finalize window layout
}

// Destructor to clean up resources
LobbyPage::~LobbyPage() {
    delete menuBar;
    delete messageDisplay;
    delete messageBuffer;
    delete messageInput;
    delete sendButton;
    delete userListBrowser;
}

// Menu callback for handling menu item actions
void LobbyPage::menuCallback(Fl_Widget* widget, void* userdata) {
    Fl_Menu_Bar* menu = static_cast<Fl_Menu_Bar*>(widget);
    LobbyPage* lobbyPage = static_cast<LobbyPage*>(userdata);
    if (!menu || !lobbyPage) return;

    const char* picked = menu->text();
    if (strcmp(picked, "Server/Disconnect") == 0) {
        lobbyPage->disconnect();
    }
    else if (strcmp(picked, "Server/Exit") == 0) {
        exit(0);
    }
    else if (strcmp(picked, "Settings/Preferences") == 0) {
        fl_alert("Settings feature is not implemented yet.");
    }
    else if (strcmp(picked, "About/About App") == 0) {
        fl_alert("Chat Client App v1.0\nDeveloped using FLTK and Winsock.");
    }
}

// Function to send messages using the Send button or Enter key
void LobbyPage::sendMessage() {
    const std::string message = messageInput->value();
    if (message.empty()) return;

    if (clientSocket) {
        try {
            clientSocket->sendMessage("message", message);
            messages.push_back("You: " + message);
            messageBuffer->text(joinMessages(messages).c_str());
            messageInput->value(""); // Clear input field
        }
        catch (const std::exception& e) {
            fl_alert("Failed to send message: %s", e.what());
        }
    }
}

// Function to handle the Send button click
void LobbyPage::sendButtonCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* lobbyPage = static_cast<LobbyPage*>(userdata);
    if (lobbyPage) {
        lobbyPage->sendMessage();
    }
}

// Override handle to capture Enter key for sending messages
int LobbyPage::handle(int event) {
    if (event == FL_KEYDOWN) {
        if (Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter) {
            sendMessage();
            return 1; // Indicate the event was handled
        }
    }
    return Fl_Window::handle(event);
}

// Function to update the user list with a new user
void LobbyPage::updateUserList(const std::string& username) {
    userListBrowser->add(username.c_str());
}

// Function to handle new incoming messages
void LobbyPage::handleNewMessage(const std::string& message) {
    messages.push_back(message);
    messageBuffer->text(joinMessages(messages).c_str());
}

// Function to disconnect from the server
void LobbyPage::disconnect() {
    if (clientSocket) {
        clientSocket->sendMessage("disconnect", "User has disconnected.");
        clientSocket.reset();
    }
    if (serverSocket) {
        serverSocket.reset();
    }
    fl_alert("Disconnected from server.");
}

// Helper to join messages into a single string
std::string LobbyPage::joinMessages(const std::vector<std::string>& messages) const {
    std::ostringstream oss;
    for (const auto& message : messages) {
        oss << message << "\n";
    }
    return oss.str();
}
// Host server function (if the user is hosting the server)
void LobbyPage::hostServer() {
    try {
        serverSocket = std::make_shared<ServerSocket>(12345); // Port 12345
        std::cout << "Server hosted at IP: " << serverSocket->getServerIP() << std::endl;
    }
    catch (const std::runtime_error& e) {
        fl_alert("Failed to host server: %s", e.what());
    }
}

// Join server function (if the user is joining a server)
void LobbyPage::joinServer(const std::string& ip, const std::string& username) {
    try {
        // Connect to the server
        clientSocket = std::make_shared<ClientSocket>(ip, 12345); // Port 12345
        clientSocket->sendMessage("username", username); // Send username to the server

        std::cout << "Connected to server at " << ip << " as " << username << std::endl;

        // Receive the list of current users
        std::string tag, message;
        if (clientSocket->receiveMessage(tag, message) && tag == "userList") {
            std::cout << "Current users in the server:\n";
            std::istringstream iss(message);
            std::string user;
            while (std::getline(iss, user, ',')) {
                std::cout << "- " << user << std::endl;
                updateUserList(user); // Add each user to the user list browser
            }
        }
    }
    catch (const std::runtime_error& e) {
        fl_alert("Failed to connect to server: %s", e.what());
    }
}


// Update function to handle periodic updates from the server
void LobbyPage::Update() {
    if (!clientSocket) return;

    std::string tag, message;
    while (clientSocket->receiveMessage(tag, message)) {
        if (tag == "broadcast") {
            handleNewMessage(message);
        }
        else if (tag == "newUser") {
            updateUserList(message);
        }
        else if (tag == "disconnect") {
            messages.push_back(message + " has disconnected.");
            messageBuffer->text(joinMessages(messages).c_str());
        }
    }
}
