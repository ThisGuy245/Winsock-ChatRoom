/**
 * @file ServerBrowser.cpp
 * @brief Implementation of the server browser sidebar
 */

#include "ServerBrowser.h"
#include "ServerManager.h"
#include "UserDatabase.h"
#include "FriendService.h"
#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>

// Color constants
namespace {
    const Fl_Color DARK_BG = fl_rgb_color(32, 34, 37);       // Discord-like dark
    const Fl_Color DARK_SIDEBAR = fl_rgb_color(47, 49, 54);
    const Fl_Color DARK_ITEM = fl_rgb_color(54, 57, 63);
    const Fl_Color DARK_HOVER = fl_rgb_color(64, 68, 75);
    const Fl_Color DARK_TEXT = FL_WHITE;
    const Fl_Color LIGHT_BG = fl_rgb_color(240, 240, 240);
    const Fl_Color LIGHT_SIDEBAR = fl_rgb_color(255, 255, 255);
    const Fl_Color LIGHT_TEXT = FL_BLACK;
    const Fl_Color ACCENT_COLOR = fl_rgb_color(88, 101, 242);
    const Fl_Color ONLINE_COLOR = fl_rgb_color(87, 242, 135);
}

ServerBrowser::ServerBrowser(int x, int y, int width, int height,
                             ServerManager* serverMgr,
                             UserDatabase* userDb,
                             FriendService* friendSvc)
    : Fl_Group(x, y, width, height)
    , serverManager(serverMgr)
    , userDatabase(userDb)
    , friendService(friendSvc)
    , currentUserId(0)
    , userInfoBox(nullptr)
    , serverListScroll(nullptr)
    , serverList(nullptr)
    , addServerButton(nullptr)
    , friendsButton(nullptr)
    , settingsButton(nullptr)
    , logoutButton(nullptr) {
    
    setupLayout();
    end();
}

ServerBrowser::~ServerBrowser() {
    // FLTK handles cleanup
}

void ServerBrowser::setupLayout() {
    int margin = 10;
    int buttonHeight = 40;
    int currentY = y() + margin;
    
    // User info at top
    userInfoBox = new Fl_Box(x() + margin, currentY, w() - 2 * margin, 50, "Not logged in");
    userInfoBox->box(FL_FLAT_BOX);
    userInfoBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    userInfoBox->labelsize(14);
    currentY += 50 + margin;
    
    // Add Server button
    addServerButton = new Fl_Button(x() + margin, currentY, w() - 2 * margin, buttonHeight, "+ Create / Join Server");
    addServerButton->box(FL_FLAT_BOX);
    addServerButton->color(ACCENT_COLOR);
    addServerButton->labelcolor(FL_WHITE);
    addServerButton->labelsize(12);
    addServerButton->callback(onAddServerClicked, this);
    currentY += buttonHeight + margin;
    
    // Server list header
    Fl_Box* serverHeader = new Fl_Box(x() + margin, currentY, w() - 2 * margin, 25, "YOUR SERVERS");
    serverHeader->labelsize(10);
    serverHeader->labelfont(FL_BOLD);
    serverHeader->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    currentY += 25;
    
    // Calculate remaining height for server list
    int bottomAreaHeight = buttonHeight * 3 + margin * 4;
    int listHeight = h() - (currentY - y()) - bottomAreaHeight;
    
    // Server list
    serverList = new Fl_Hold_Browser(x() + margin, currentY, w() - 2 * margin, listHeight);
    serverList->textsize(13);
    serverList->callback(onServerListSelected, this);
    currentY += listHeight + margin;
    
    // Friends button
    friendsButton = new Fl_Button(x() + margin, currentY, w() - 2 * margin, buttonHeight, "Friends");
    friendsButton->box(FL_FLAT_BOX);
    friendsButton->labelsize(12);
    friendsButton->callback(onFriendsClicked, this);
    currentY += buttonHeight + margin;
    
    // Settings button (placeholder)
    settingsButton = new Fl_Button(x() + margin, currentY, w() - 2 * margin, buttonHeight, "Settings");
    settingsButton->box(FL_FLAT_BOX);
    settingsButton->labelsize(12);
    settingsButton->callback(onSettingsClicked, this);
    currentY += buttonHeight + margin;
    
    // Logout button
    logoutButton = new Fl_Button(x() + margin, currentY, w() - 2 * margin, buttonHeight, "Logout");
    logoutButton->box(FL_FLAT_BOX);
    logoutButton->labelsize(12);
    logoutButton->callback(onLogoutClicked, this);
}

void ServerBrowser::setCurrentUser(uint64_t userId, const std::string& username) {
    currentUserId = userId;
    currentUsername = username;
    
    // Update user info display
    std::string displayText = username;
    userInfoBox->copy_label(displayText.c_str());
    
    refreshServerList();
    updateFriendBadge();
}

void ServerBrowser::refreshServerList() {
    if (currentUserId == 0 || !serverManager) {
        return;
    }
    
    serverList->clear();
    cachedServers = serverManager->GetUserServers(currentUserId);
    
    for (const Models::ChatServer& server : cachedServers) {
        std::string displayName = server.serverName;
        
        // Indicate if user is owner
        if (server.ownerId == currentUserId) {
            displayName = server.serverName + " (Owner)";
        }
        
        serverList->add(displayName.c_str());
    }
    
    if (cachedServers.empty()) {
        serverList->add("@i@- No servers joined -");
    }
}

void ServerBrowser::setOnServerSelected(ServerSelectedCallback callback) {
    onServerSelectedCallback = callback;
}

void ServerBrowser::setOnFriendsClicked(FriendsClickedCallback callback) {
    onFriendsClickedCallback = callback;
}

void ServerBrowser::applyTheme(bool isDarkMode) {
    if (isDarkMode) {
        color(DARK_SIDEBAR);
        userInfoBox->color(DARK_SIDEBAR);
        userInfoBox->labelcolor(DARK_TEXT);
        
        serverList->color(DARK_ITEM);
        serverList->textcolor(DARK_TEXT);
        serverList->selection_color(ACCENT_COLOR);
        
        friendsButton->color(DARK_ITEM);
        friendsButton->labelcolor(DARK_TEXT);
        settingsButton->color(DARK_ITEM);
        settingsButton->labelcolor(DARK_TEXT);
        logoutButton->color(DARK_ITEM);
        logoutButton->labelcolor(DARK_TEXT);
    } else {
        color(LIGHT_SIDEBAR);
        userInfoBox->color(LIGHT_SIDEBAR);
        userInfoBox->labelcolor(LIGHT_TEXT);
        
        serverList->color(FL_WHITE);
        serverList->textcolor(LIGHT_TEXT);
        serverList->selection_color(ACCENT_COLOR);
        
        friendsButton->color(LIGHT_BG);
        friendsButton->labelcolor(LIGHT_TEXT);
        settingsButton->color(LIGHT_BG);
        settingsButton->labelcolor(LIGHT_TEXT);
        logoutButton->color(LIGHT_BG);
        logoutButton->labelcolor(LIGHT_TEXT);
    }
    
    redraw();
}

void ServerBrowser::showCreateServerDialog() {
    const char* serverName = fl_input("Enter server name:", "My Server");
    
    if (serverName && strlen(serverName) > 0) {
        Models::ChatServer newServer;
        Protocol::ErrorCode result = serverManager->CreateServer(
            serverName, currentUserId, newServer
        );
        
        if (result == Protocol::ErrorCode::None) {
            fl_message("Server '%s' created successfully!", serverName);
            refreshServerList();
            
            // Auto-select the new server
            if (onServerSelectedCallback) {
                onServerSelectedCallback(newServer.serverId, newServer.serverName);
            }
        } else {
            fl_alert("Failed to create server: %s", 
                     Protocol::ErrorCodeToMessage(result));
        }
    }
}

void ServerBrowser::showJoinServerDialog() {
    // For now, show a simple search dialog
    const char* searchTerm = fl_input("Search for a server to join:", "");
    
    if (searchTerm && strlen(searchTerm) > 0) {
        std::vector<Models::ChatServer> results = serverManager->SearchServers(searchTerm, 10);
        
        if (results.empty()) {
            fl_message("No servers found matching '%s'", searchTerm);
            return;
        }
        
        // Show first result (simplified - in full version would show list)
        const Models::ChatServer& server = results[0];
        
        int choice = fl_choice("Found server: %s\n\nJoin this server?",
                               "Cancel", "Join", nullptr,
                               server.serverName.c_str());
        
        if (choice == 1) {
            Protocol::ErrorCode result = serverManager->JoinServer(server.serverId, currentUserId);
            
            if (result == Protocol::ErrorCode::None) {
                fl_message("Joined server '%s'!", server.serverName.c_str());
                refreshServerList();
            } else {
                fl_alert("Failed to join: %s", Protocol::ErrorCodeToMessage(result));
            }
        }
    }
}

void ServerBrowser::updateFriendBadge() {
    if (!friendService || currentUserId == 0) {
        return;
    }
    
    size_t pendingCount = friendService->GetPendingRequestCount(currentUserId);
    
    if (pendingCount > 0) {
        char label[64];
        snprintf(label, sizeof(label), "Friends (%zu)", pendingCount);
        friendsButton->copy_label(label);
    } else {
        friendsButton->label("Friends");
    }
}

// Static callbacks

void ServerBrowser::onServerListSelected(Fl_Widget* widget, void* userdata) {
    ServerBrowser* browser = static_cast<ServerBrowser*>(userdata);
    
    int selected = browser->serverList->value();
    if (selected <= 0 || selected > static_cast<int>(browser->cachedServers.size())) {
        return;
    }
    
    // Adjust for 1-based indexing
    const Models::ChatServer& server = browser->cachedServers[selected - 1];
    
    if (browser->onServerSelectedCallback) {
        browser->onServerSelectedCallback(server.serverId, server.serverName);
    }
}

void ServerBrowser::onAddServerClicked(Fl_Widget* widget, void* userdata) {
    ServerBrowser* browser = static_cast<ServerBrowser*>(userdata);
    
    // Ask user what they want to do
    int choice = fl_choice("What would you like to do?",
                           "Cancel", "Create Server", "Join Server");
    
    if (choice == 1) {
        browser->showCreateServerDialog();
    } else if (choice == 2) {
        browser->showJoinServerDialog();
    }
}

void ServerBrowser::onFriendsClicked(Fl_Widget* widget, void* userdata) {
    ServerBrowser* browser = static_cast<ServerBrowser*>(userdata);
    
    if (browser->onFriendsClickedCallback) {
        browser->onFriendsClickedCallback();
    }
}

void ServerBrowser::onSettingsClicked(Fl_Widget* widget, void* userdata) {
    // Placeholder - will open settings
    fl_message("Settings coming soon!");
}

void ServerBrowser::onLogoutClicked(Fl_Widget* widget, void* userdata) {
    int confirm = fl_choice("Are you sure you want to logout?",
                            "Cancel", "Logout", nullptr);
    
    if (confirm == 1) {
        // This should trigger a callback to main window to handle logout
        // For now, just notify
        fl_message("Logout requested");
    }
}
