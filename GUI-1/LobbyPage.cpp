#include "LobbyPage.hpp"
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Display.H>  // For displaying chat messages
#include <FL/Fl_Text_Buffer.H>    // For managing text buffer

LobbyPage::LobbyPage(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H), client(nullptr), server(nullptr) {

    // Input field for typing messages
    messageInput = new Fl_Input(W / 2 - 150, H - 50, 300, 30, "Message:");
    sendButton = new Fl_Button(W / 2 - 50, H - 50, 100, 30, "Send");
    sendButton->callback([](Fl_Widget* widget, void* userdata) {
        LobbyPage* page = static_cast<LobbyPage*>(userdata);
        if (page) {
            const std::string message = page->messageInput->value();
            page->sendMessage(message);
            page->messageInput->value("");  // Clear input after sending
        }
        }, this);

    // Display area for chat history
    chatDisplay = new Fl_Text_Display(10, 10, W - 20, H - 80);
    Fl_Text_Buffer* buffer = new Fl_Text_Buffer();
    chatDisplay->buffer(buffer);

    end();
}

LobbyPage::~LobbyPage() {
    delete client;
    delete server;
}

void LobbyPage::hostServer(const std::string& ip, const std::string& username) {
    // Host the server with the provided IP and username
    server = new ServerSocket(12345);  // Example port
    // Initialize server and wait for client connections
    // After server setup, run the handleClientConnections loop in Update()
}

void LobbyPage::joinServer(const std::string& ip, const std::string& username) {
    // Connect as a client to a server
    client = new ClientSocket(ip, 12345);  // Example port
    // Set up the client to send/receive messages
}

void LobbyPage::sendMessage(const std::string& message) {
    if (client) {
        client->send(message);
    }
}

void LobbyPage::receiveMessages() {
    std::string message;
    if (client && client->receive(message)) {
        // Display the received message
        Fl_Text_Buffer* buffer = chatDisplay->buffer();
        buffer->append(message.c_str());
    }
    if (server) {
        server->handleClientConnections();  // Handle new client connections and messages
    }
}

void LobbyPage::Update() {
    receiveMessages();  // Call periodically to receive messages
}
