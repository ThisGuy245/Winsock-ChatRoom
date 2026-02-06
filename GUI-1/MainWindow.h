#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <FL/Fl_Window.H>
#include <functional>
#include <vector>
#include <memory>
#include "Timer.h"
#include "HomePage.hpp"
#include "LobbyPage.hpp"
#include "LoginPage.h"
#include "ServerBrowser.h"
#include "ChannelList.h"
#include "UserDatabase.h"
#include "ServerManager.h"
#include "FriendService.h"
#include "MessageService.h"

// Forward declarations
class HomePage;
class LobbyPage;
class LoginPage;
class ServerBrowser;
class ChannelList;

/**
 * @class MainWindow
 * @brief Main window managing application pages and services
 * 
 * Page flow for Discord-like functionality:
 * 1. LoginPage -> User registers/logs in
 * 2. ServerBrowser -> User selects a server
 * 3. ChannelList + LobbyPage -> User chats in channels
 */
class MainWindow : public Fl_Window {
public:
    MainWindow(int width, int height);
    ~MainWindow();

    void setResolution(int width, int height);
    LobbyPage* getLobbyPage() const;
    
    // Page switching
    static void switch_to_home(Fl_Widget* widget, void* userdata);
    static void switch_to_lobby(Fl_Widget* widget, void* userdata);
    void switchToLogin();
    void switchToServerBrowser();
    void switchToChat(uint64_t serverId, const std::string& serverName);
    
    void on_close(const std::function<void()>& callback);
    void close();
    void resize(int X, int Y, int W, int H);
    void onTick(void* userdata);
    
    // Apply theme to all pages
    void applyThemeToAll(bool isDarkMode);

    // Pages
    HomePage* homePage;
    LobbyPage* lobbyPage;
    LoginPage* loginPage;
    ServerBrowser* serverBrowser;
    ChannelList* channelList;
    
    // Services (owned by MainWindow)
    std::unique_ptr<UserDatabase> userDatabase;
    std::unique_ptr<ServerManager> serverManager;
    std::unique_ptr<FriendService> friendService;
    std::unique_ptr<MessageService> messageService;
    
    // Current user session
    uint64_t currentUserId;
    std::string currentUsername;
    std::string currentSessionToken;
    
    // Current server/channel
    uint64_t currentServerId;
    uint64_t currentChannelId;
    bool isHostingServer;
    
    std::vector<std::function<void()>> close_callbacks;

    // Networking methods
    void startHostingServer(uint64_t serverId);
    void connectToServer(uint64_t serverId);
    void disconnectFromCurrentServer();
    std::string getLocalIPAddress();

private:
    Timer timer;
    
    void initializeServices();
    void setupPageCallbacks();
};

#endif // MAINWINDOW_H
