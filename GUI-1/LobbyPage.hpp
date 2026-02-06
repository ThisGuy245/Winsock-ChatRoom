#ifndef LOBBYPAGE_HPP
#define LOBBYPAGE_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>

#include <string>
#include <functional>
#include "ClientSocket.h"
#include "ServerSocket.h"
#include "PlayerDisplay.hpp"
#include "SettingsWindow.hpp"
#include "AboutWindow.h"
#include "MessageService.h"

// Forward declarations
class SettingsWindow;
class MainWindow;

class LobbyPage : public Fl_Group {
public:
    LobbyPage(int X, int Y, int W, int H);
    ~LobbyPage();

    // Original methods with parameters (legacy)
    void hostServer(const std::string& ip, const std::string& username);
    void joinServer(const std::string& ip, const std::string& username);
    
    // New networking integration methods (uses internal state)
    void hostServer();     // Host using stored IP/port
    void joinServer();     // Join using stored IP/port
    void disconnectAndReset();  // Disconnect and clean up
    
    // Getters for network inputs
    Fl_Input* getIpInput() { return ipInput; }
    Fl_Input* getPortInput() { return portInput; }
    
    void Update();  // Network update
    void sendMessage(const std::string& message);
    void receiveMessages();
    void setUsername(const std::string& user) { username = user; }
    std::string getUsername() const { return username; }
    void setServerName(const std::string& name);
    void setChannelName(const std::string& name);
    void setCurrentChannel(uint64_t channelId) { currentChannelId = channelId; }
    uint64_t getCurrentChannel() const { return currentChannelId; }
    void clientLeft(const std::string& username);
    void resizeWidgets(int X, int Y, int W, int H);
    void changeUsername(const std::string& newUsername);
    void cleanupSession();
    
    // Channel-based message history
    void setMessageService(MessageService* service) { messageService = service; }
    void loadChannelHistory(uint64_t channelId, MessageService* service);
    void saveMessageToHistory(const std::string& senderName, const std::string& content);
    void saveSystemMessageToHistory(const std::string& content);
    
    // Theme support
    void applyTheme(bool isDarkMode);
    
    // Connection state
    bool isConnected() const { return client != nullptr || server != nullptr; }
    bool isHosting() const { return server != nullptr; }
    
    // UI Control - for when ChannelList sidebar is shown
    void setHeaderVisible(bool visible);
    void setMemberPanelVisible(bool visible);

    // UI Components - Header
    Fl_Group* headerBar;
    Fl_Button* backButton;
    Fl_Box* serverNameLabel;
    Fl_Box* channelNameLabel;
    Fl_Button* settingsButton;
    Fl_Button* aboutButton;
    
    // UI Components - Main Area
    Fl_Group* mainArea;
    Fl_Group* chatArea;
    Fl_Text_Display* chatDisplay;
    Fl_Text_Buffer* chatBuffer;
    Fl_Scroll* scrollArea;
    
    // UI Components - Member List
    Fl_Group* memberPanel;
    Fl_Box* memberHeaderLabel;
    PlayerDisplay* playerDisplay;
    
    // UI Components - Message Input
    Fl_Group* inputBar;
    Fl_Input* messageInput;
    Fl_Button* sendButton;
    
    // Hidden network config inputs (used programmatically)
    Fl_Input* ipInput;
    Fl_Input* portInput;

    // Network components
    ClientSocket* client;
    ServerSocket* server;
    
    // Windows
    SettingsWindow* settings;
    AboutWindow* about;
    
    // Callbacks
    void setOnBackClicked(std::function<void()> callback) { onBackClicked = callback; }
    void setOnSettingsClicked(std::function<void()> callback) { onSettingsClicked = callback; }

private:
    std::string username;
    std::string currentServerName;
    std::string currentChannelName;
    uint64_t currentChannelId;
    uint16_t currentPort;
    bool darkMode;
    
    // Message service for persistent history
    MessageService* messageService;
    
    std::function<void()> onBackClicked;
    std::function<void()> onSettingsClicked;
    
    // Internal callbacks
    static void backButtonCallback(Fl_Widget* widget, void* userdata);
    static void settingsButtonCallback(Fl_Widget* widget, void* userdata);
    static void aboutButtonCallback(Fl_Widget* widget, void* userdata);
    static void sendButtonCallback(Fl_Widget* widget, void* userdata);
    static void messageInputCallback(Fl_Widget* widget, void* userdata);
    
    // Theme colors
    void updateColors();
};

#endif // LOBBYPAGE_HPP
