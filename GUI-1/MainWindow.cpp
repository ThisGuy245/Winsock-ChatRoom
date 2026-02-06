#include "MainWindow.h"
#include "HomePage.hpp"
#include "LobbyPage.hpp"
#include <FL/fl_ask.H>
#include <memory>
#include "Settings.h"

// For getting local IP
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

/**
 * @class MainWindow
 * @brief The main application window managing different pages and services
 */

MainWindow::MainWindow(int width, int height)
    : Fl_Window(width, height)
    , homePage(nullptr)
    , lobbyPage(nullptr)
    , loginPage(nullptr)
    , serverBrowser(nullptr)
    , channelList(nullptr)
    , currentUserId(0)
    , currentServerId(0)
    , currentChannelId(0)
    , isHostingServer(false)
    , timer(0.1) {

    // Load resolution from settings XML
    Settings settings("config.xml");
    std::string username = settings.getUsername();
    pugi::xml_node userNode = settings.findClient(username);
    auto [savedWidth, savedHeight] = settings.getRes(userNode);

    // Resize window with the saved resolution
    resize(0, 0, savedWidth, savedHeight);

    // Make the window resizable
    resizable(this);

    // Initialize services first
    initializeServices();

    // Initialize LoginPage (shown first for new Discord-like flow)
    loginPage = new LoginPage(0, 0, savedWidth, savedHeight, userDatabase.get());
    if (!loginPage) {
        fl_alert("Failed to create LoginPage!");
        return;
    }
    add(loginPage);
    loginPage->hide();  // Will be shown after setup

    // Initialize homePage (legacy - for direct IP connection)
    homePage = new HomePage(0, 0, savedWidth, savedHeight, this);
    if (!homePage) {
        fl_alert("Failed to create HomePage!");
        return;
    }
    add(homePage);
    homePage->hide();

    // Initialize ServerBrowser
    serverBrowser = new ServerBrowser(0, 0, 250, savedHeight,
                                       serverManager.get(),
                                       userDatabase.get(),
                                       friendService.get());
    if (!serverBrowser) {
        fl_alert("Failed to create ServerBrowser!");
        return;
    }
    add(serverBrowser);
    serverBrowser->hide();

    // Initialize ChannelList
    channelList = new ChannelList(0, 0, 200, savedHeight,
                                   serverManager.get(),
                                   userDatabase.get());
    if (!channelList) {
        fl_alert("Failed to create ChannelList!");
        return;
    }
    add(channelList);
    channelList->hide();

    // Initialize lobbyPage (chat area)
    lobbyPage = new LobbyPage(0, 0, savedWidth, savedHeight);
    if (!lobbyPage) {
        fl_alert("Failed to create LobbyPage!");
        return;
    }
    add(lobbyPage);
    lobbyPage->hide();

    // Setup callbacks between pages
    setupPageCallbacks();

    // End adding widgets to this window
    end();

    // Start with the new Discord-like login flow
    loginPage->show();

    // Timer for periodic updates
    timer.setCallback([](void* userdata) {
        auto* window = static_cast<MainWindow*>(userdata);
        if (window) {
            window->onTick(userdata);
        }
    });
    timer.setUserData(this);
    timer.start();

    // Set the minimum window size
    size_range(800, 600, 10000, 10000);
}

MainWindow::~MainWindow() {
    // Disconnect from any active server
    disconnectFromCurrentServer();
    
    delete homePage;
    delete lobbyPage;
    delete loginPage;
    delete serverBrowser;
    delete channelList;
    close();
}

void MainWindow::initializeServices() {
    // Create database directory if needed
    userDatabase = std::make_unique<UserDatabase>("user_data.xml");
    serverManager = std::make_unique<ServerManager>("server_data.xml", *userDatabase);
    friendService = std::make_unique<FriendService>("friend_data.xml", *userDatabase);
    messageService = std::make_unique<MessageService>("message_history.xml");
}

void MainWindow::setupPageCallbacks() {
    // LoginPage callback - when user successfully logs in
    if (loginPage) {
        loginPage->setOnAuthenticated([this](uint64_t userId, const std::string& username, const std::string& token) {
            currentUserId = userId;
            currentUsername = username;
            currentSessionToken = token;
            
            printf("[APP] User authenticated: %s\n", username.c_str());
            
            switchToServerBrowser();
        });
    }
    
    // ServerBrowser callback - when user selects a server
    if (serverBrowser) {
        serverBrowser->setOnServerSelected([this](uint64_t serverId, const std::string& serverName) {
            printf("[APP] Server selected: %s\n", serverName.c_str());
            switchToChat(serverId, serverName);
        });
        
        serverBrowser->setOnFriendsClicked([this]() {
            fl_message("Friends panel coming soon!");
        });
    }
    
    // ChannelList callback - when user goes back to server list
    if (channelList) {
        channelList->setOnBackClicked([this]() {
            // Disconnect from current server before going back
            disconnectFromCurrentServer();
            switchToServerBrowser();
        });
        
        channelList->setOnChannelSelected([this](uint64_t channelId, const std::string& channelName) {
            printf("[APP] Channel selected: #%s (ID: %llu)\n", channelName.c_str(), channelId);
            
            // Load channel history
            if (lobbyPage && messageService) {
                // Update channel name in UI
                lobbyPage->setChannelName(channelName);
                
                // Store current channel ID
                currentChannelId = channelId;
                
                // Load this channel's message history
                lobbyPage->loadChannelHistory(channelId, messageService.get());
            }
        });
    }
    
    // LobbyPage callback - when user clicks back button
    if (lobbyPage) {
        lobbyPage->setOnBackClicked([this]() {
            // Disconnect and go back to server browser
            disconnectFromCurrentServer();
            switchToServerBrowser();
        });
    }
}

void MainWindow::switchToLogin() {
    if (homePage) homePage->hide();
    if (lobbyPage) lobbyPage->hide();
    if (serverBrowser) serverBrowser->hide();
    if (channelList) channelList->hide();
    
    if (loginPage) {
        loginPage->clearFields();
        loginPage->resize(0, 0, w(), h());
        loginPage->show();
    }
    
    redraw();
}

void MainWindow::switchToServerBrowser() {
    // Disconnect from any active server connection
    disconnectFromCurrentServer();
    
    if (homePage) homePage->hide();
    if (lobbyPage) lobbyPage->hide();
    if (loginPage) loginPage->hide();
    if (channelList) channelList->hide();
    
    // Clean up lobby page for next use
    if (lobbyPage) {
        lobbyPage->cleanupSession();
    }
    
    if (serverBrowser) {
        serverBrowser->setCurrentUser(currentUserId, currentUsername);
        serverBrowser->refreshServerList();
        serverBrowser->resize(0, 0, 250, h());
        serverBrowser->show();
    }
    
    redraw();
}

void MainWindow::switchToChat(uint64_t serverId, const std::string& serverName) {
    if (homePage) homePage->hide();
    if (loginPage) loginPage->hide();
    
    currentServerId = serverId;
    
    // Get server info to determine if we're the owner
    Models::ChatServer server;
    if (!serverManager->GetServer(serverId, server)) {
        fl_alert("Failed to get server information!");
        return;
    }
    
    // Clean up any previous session
    if (lobbyPage) {
        lobbyPage->cleanupSession();
    }
    
    // Set the username for chat
    if (lobbyPage) {
        lobbyPage->setUsername(currentUsername);
    }
    
    // Determine if we're owner (host) or member (client)
    bool isOwner = serverManager->IsServerOwner(serverId, currentUserId);
    
    if (isOwner) {
        // Owner hosts the server
        startHostingServer(serverId);
    } else {
        // Member connects as client
        connectToServer(serverId);
    }
    
    // Show sidebar layout: ChannelList + LobbyPage
    int sidebarWidth = 200;
    int chatX = sidebarWidth;
    int chatWidth = w() - sidebarWidth;
    
    // Hide server browser, show channel list
    if (serverBrowser) serverBrowser->hide();
    
    if (channelList) {
        channelList->setServer(serverId, currentUserId);
        channelList->resize(0, 0, sidebarWidth, h());
        channelList->show();
    }
    
    if (lobbyPage) {
        lobbyPage->setServerName(serverName);
        lobbyPage->setMessageService(messageService.get());
        
        // Get the server's default channel (first channel, usually "general")
        auto channels = serverManager->GetServerChannels(serverId);
        if (!channels.empty()) {
            uint64_t defaultChannelId = channels[0].channelId;
            std::string defaultChannelName = channels[0].channelName;
            
            currentChannelId = defaultChannelId;
            lobbyPage->setChannelName(defaultChannelName);
            lobbyPage->setCurrentChannel(defaultChannelId);
            
            // Load channel history
            lobbyPage->loadChannelHistory(defaultChannelId, messageService.get());
            
            printf("[APP] Default channel: #%s (ID: %llu)\n", defaultChannelName.c_str(), defaultChannelId);
        } else {
            lobbyPage->setChannelName("general");
        }
        
        lobbyPage->resize(chatX, 0, chatWidth, h());
        lobbyPage->resizeWidgets(chatX, 0, chatWidth, h());
        lobbyPage->show();
        
        // Apply current theme
        Settings themeSettings("settings.xml");
        auto userNode = themeSettings.findClient(currentUsername);
        bool isDark = (themeSettings.getMode(userNode) == "dark");
        lobbyPage->applyTheme(isDark);
    }
    
    redraw();
}

void MainWindow::resize(int X, int Y, int W, int H) {
    Fl_Window::resize(X, Y, W, H);

    // Save the resolution to settings
    Settings settings("settings.xml");
    settings.setRes(settings.getUsername(), W, H);

    if (homePage && homePage->visible()) {
        homePage->resize(0, 0, W, H);
    }
    if (loginPage && loginPage->visible()) {
        loginPage->resize(0, 0, W, H);
    }
    if (serverBrowser && serverBrowser->visible()) {
        serverBrowser->resize(0, 0, 250, H);
    }
    if (channelList && channelList->visible() && lobbyPage && lobbyPage->visible()) {
        int sidebarWidth = 200;
        channelList->resize(0, 0, sidebarWidth, H);
        lobbyPage->resize(sidebarWidth, 0, W - sidebarWidth, H);
        lobbyPage->resizeWidgets(sidebarWidth, 0, W - sidebarWidth, H);
    } else if (lobbyPage && lobbyPage->visible()) {
        lobbyPage->resize(0, 0, W, H);
        lobbyPage->resizeWidgets(0, 0, W, H);
    }
}

void MainWindow::onTick(void* userdata) {
    if (lobbyPage && lobbyPage->visible()) {
        lobbyPage->Update();
    }

    // Repeat the timer callback
    Fl::repeat_timeout(1.0, [](void* userdata) {
        auto* window = static_cast<MainWindow*>(userdata);
        if (window) {
            window->onTick(userdata);
        }
    }, userdata);
}

void MainWindow::setResolution(int width, int height) {
    this->size(width, height);
    this->redraw();
}

LobbyPage* MainWindow::getLobbyPage() const {
    return lobbyPage;
}

void MainWindow::switch_to_home(Fl_Widget* widget, void* userdata) {
    auto* window = static_cast<MainWindow*>(userdata);
    if (!window) {
        fl_alert("Failed to retrieve MainWindow instance for switching pages.");
        return;
    }

    if (window->lobbyPage) {
        window->lobbyPage->hide();
    }
    if (window->serverBrowser) {
        window->serverBrowser->hide();
    }
    if (window->channelList) {
        window->channelList->hide();
    }
    if (window->loginPage) {
        window->loginPage->hide();
    }
    if (window->homePage) {
        window->homePage->resize(0, 0, window->w(), window->h());
        window->homePage->show();
    }
    window->redraw();
}

void MainWindow::switch_to_lobby(Fl_Widget* widget, void* userdata) {
    auto* window = static_cast<MainWindow*>(userdata);
    if (!window) {
        fl_alert("Failed to retrieve MainWindow instance for switching pages.");
        return;
    }

    if (window->homePage) {
        window->homePage->hide();
    }
    if (window->loginPage) {
        window->loginPage->hide();
    }
    if (window->serverBrowser) {
        window->serverBrowser->hide();
    }
    if (window->channelList) {
        window->channelList->hide();
    }
    if (window->lobbyPage) {
        window->lobbyPage->resize(0, 0, window->w(), window->h());
        window->lobbyPage->resizeWidgets(0, 0, window->w(), window->h());
        window->lobbyPage->show();
    } else {
        fl_alert("LobbyPage is not initialized!");
    }
    window->redraw();
}

void MainWindow::on_close(const std::function<void()>& callback) {
    close_callbacks.push_back(callback);
}

void MainWindow::close() {
    for (const auto& callback : close_callbacks) {
        callback();
    }
    hide();
}

void MainWindow::applyThemeToAll(bool isDarkMode) {
    if (homePage) {
        homePage->applyTheme(isDarkMode);
    }
    if (loginPage) {
        loginPage->applyTheme(isDarkMode);
    }
    if (serverBrowser) {
        serverBrowser->applyTheme(isDarkMode);
    }
    if (channelList) {
        channelList->applyTheme(isDarkMode);
    }
    if (lobbyPage) {
        lobbyPage->applyTheme(isDarkMode);
    }
    redraw();
}

//=============================================================================
// NETWORKING INTEGRATION
//=============================================================================

std::string MainWindow::getLocalIPAddress() {
    // Get the local IP address for hosting
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        return "127.0.0.1";
    }
    
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;
    
    struct addrinfo* result = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
        return "127.0.0.1";
    }
    
    std::string ipAddress = "127.0.0.1";
    for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
        
        // Skip loopback addresses
        if (strncmp(ipStr, "127.", 4) != 0) {
            ipAddress = ipStr;
            break;
        }
    }
    
    freeaddrinfo(result);
    return ipAddress;
}

void MainWindow::startHostingServer(uint64_t serverId) {
    printf("[NET] Starting to host server (ID: %llu)\n", serverId);
    
    // Get server info
    Models::ChatServer server;
    if (!serverManager->GetServer(serverId, server)) {
        fl_alert("Failed to get server information!");
        return;
    }
    
    // Get local IP and set port
    std::string localIP = getLocalIPAddress();
    uint16_t port = server.hostPort > 0 ? server.hostPort : Models::ChatServer::DEFAULT_PORT;
    
    // Update server network info in database
    serverManager->SetServerNetworkInfo(serverId, localIP, port);
    
    // Use LobbyPage's existing hosting functionality
    if (lobbyPage) {
        // The LobbyPage has a serverSocket that we can use
        // Set the port and start hosting
        std::string portStr = std::to_string(port);
        lobbyPage->getPortInput()->value(portStr.c_str());
        
        // Trigger host action
        lobbyPage->hostServer();
        
        isHostingServer = true;
        serverManager->SetServerOnlineStatus(serverId, true);
        
        printf("[NET] Hosting on %s:%d\n", localIP.c_str(), port);
    }
}

void MainWindow::connectToServer(uint64_t serverId) {
    printf("[NET] Connecting to server (ID: %llu)\n", serverId);
    
    // Get server info
    Models::ChatServer server;
    if (!serverManager->GetServer(serverId, server)) {
        fl_alert("Failed to get server information!");
        return;
    }
    
    // Check if server is online and has network info
    if (!server.isOnline || server.hostIpAddress.empty()) {
        fl_alert("Server is not online. The owner must be hosting for you to connect.");
        switchToServerBrowser();
        return;
    }
    
    // Use LobbyPage's existing client connection functionality
    if (lobbyPage) {
        // Set IP and port
        lobbyPage->getIpInput()->value(server.hostIpAddress.c_str());
        lobbyPage->getPortInput()->value(std::to_string(server.hostPort).c_str());
        
        // Trigger join action
        lobbyPage->joinServer();
        
        isHostingServer = false;
        
        printf("[NET] Connecting to %s:%d\n", server.hostIpAddress.c_str(), server.hostPort);
    }
}

void MainWindow::disconnectFromCurrentServer() {
    if (currentServerId == 0) {
        return;
    }
    
    printf("[NET] Disconnecting from server (ID: %llu)\n", currentServerId);
    
    if (isHostingServer) {
        // Stop hosting
        if (lobbyPage) {
            lobbyPage->disconnectAndReset();
        }
        serverManager->SetServerOnlineStatus(currentServerId, false);
        printf("[NET] Stopped hosting\n");
    } else {
        // Disconnect as client
        if (lobbyPage) {
            lobbyPage->disconnectAndReset();
        }
        printf("[NET] Disconnected from server\n");
    }
    
    currentServerId = 0;
    currentChannelId = 0;
    isHostingServer = false;
}
