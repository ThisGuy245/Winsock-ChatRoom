#ifndef LOBBYPAGE_HPP
#define LOBBYPAGE_HPP

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Menu_Bar.H>
#include <memory>
#include <vector>
#include <string>
#include "ClientSocket.h"
#include "ServerSocket.h"

class LobbyPage : public Fl_Window {
private:
    // UI Components
    Fl_Menu_Bar* menuBar;                 // Menu bar at the top
    Fl_Text_Display* messageDisplay;      // Area for displaying messages
    Fl_Text_Buffer* messageBuffer;        // Buffer for text in the message display
    Fl_Input* messageInput;               // Input field for typing messages
    Fl_Button* sendButton;                // Button to send messages
    Fl_Hold_Browser* userListBrowser;     // Browser to display the list of users

    // Networking
    std::shared_ptr<ClientSocket> clientSocket; // Client socket for communication
    std::shared_ptr<ServerSocket> serverSocket; // Server socket for hosting

    // Data Storage
    std::vector<std::string> messages;    // Stores chat messages

    // Private Methods
    static void sendButtonCallback(Fl_Widget* widget, void* userdata); // Send button callback
    static void menuCallback(Fl_Widget* widget, void* userdata);       // Menu bar callback
    void sendMessage();                     // Function to send a message
    std::string joinMessages(const std::vector<std::string>& messages) const; // Helper to join messages

public:
    // Constructor and Destructor
    LobbyPage(int X, int Y, int W, int H);  // Constructor
    ~LobbyPage();                           // Destructor

    // Public Methods
    void Update();                          // Called by the timer to refresh UI
    void handleNewMessage(const std::string& message); // Handle incoming messages
    void updateUserList(const std::string& username);  // Update user list
    void disconnect();                      // Disconnect from the server
    void hostServer();                      // Start hosting a server
    void joinServer(const std::string& ip, const std::string& username); // Join an existing server

    // Override FLTK handle for key press detection
    int handle(int event) override;
};

#endif // LOBBYPAGE_HPP
