#ifndef SERVER_BROWSER_H
#define SERVER_BROWSER_H

/**
 * @file ServerBrowser.h
 * @brief Server browser for viewing and managing chat servers
 * 
 * Displays:
 * - User's joined servers in a sidebar
 * - Server creation dialog
 * - Server search/discovery
 * - Friend list quick access
 */

#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Hold_Browser.H>
#include <functional>
#include <vector>
#include <string>
#include "Models.h"

// Forward declarations
class ServerManager;
class UserDatabase;
class FriendService;

class ServerBrowser : public Fl_Group {
public:
    /**
     * @brief Callback when user selects a server
     * @param serverId The selected server's ID
     * @param serverName The selected server's name
     */
    using ServerSelectedCallback = std::function<void(uint64_t serverId, const std::string& serverName)>;
    
    /**
     * @brief Callback when user wants to view friends
     */
    using FriendsClickedCallback = std::function<void()>;
    
    ServerBrowser(int x, int y, int width, int height,
                  ServerManager* serverManager,
                  UserDatabase* userDatabase,
                  FriendService* friendService);
    ~ServerBrowser();
    
    /**
     * @brief Set the current logged-in user
     */
    void setCurrentUser(uint64_t userId, const std::string& username);
    
    /**
     * @brief Refresh the server list
     */
    void refreshServerList();
    
    /**
     * @brief Set callback for server selection
     */
    void setOnServerSelected(ServerSelectedCallback callback);
    
    /**
     * @brief Set callback for friends button
     */
    void setOnFriendsClicked(FriendsClickedCallback callback);
    
    /**
     * @brief Apply dark/light theme
     */
    void applyTheme(bool isDarkMode);
    
    /**
     * @brief Show create server dialog
     */
    void showCreateServerDialog();
    
    /**
     * @brief Show join server dialog
     */
    void showJoinServerDialog();
    
private:
    ServerManager* serverManager;
    UserDatabase* userDatabase;
    FriendService* friendService;
    
    uint64_t currentUserId;
    std::string currentUsername;
    
    ServerSelectedCallback onServerSelectedCallback;
    FriendsClickedCallback onFriendsClickedCallback;
    
    // UI Components - Sidebar style
    Fl_Box* userInfoBox;           // Shows logged-in user info
    Fl_Scroll* serverListScroll;   // Scrollable server list
    Fl_Hold_Browser* serverList;   // Server list widget
    Fl_Button* addServerButton;    // Create/Join server
    Fl_Button* friendsButton;      // Quick access to friends
    Fl_Button* settingsButton;     // User settings
    Fl_Button* logoutButton;       // Logout
    
    // Server data cache
    std::vector<Models::ChatServer> cachedServers;
    
    // Layout helpers
    void setupLayout();
    
    // Callbacks
    static void onServerListSelected(Fl_Widget* widget, void* userdata);
    static void onAddServerClicked(Fl_Widget* widget, void* userdata);
    static void onFriendsClicked(Fl_Widget* widget, void* userdata);
    static void onSettingsClicked(Fl_Widget* widget, void* userdata);
    static void onLogoutClicked(Fl_Widget* widget, void* userdata);
    
    // Update friend notification badge
    void updateFriendBadge();
};

#endif // SERVER_BROWSER_H
