#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H

/**
 * @file ChannelList.h
 * @brief Channel list sidebar for a selected server
 * 
 * Shows:
 * - Server name and options
 * - List of text channels
 * - Online members list
 * - Channel management for owners
 */

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Scroll.H>
#include <functional>
#include <vector>
#include "Models.h"

// Forward declarations
class ServerManager;
class UserDatabase;

class ChannelList : public Fl_Group {
public:
    /**
     * @brief Callback when channel is selected
     */
    using ChannelSelectedCallback = std::function<void(uint64_t channelId, const std::string& channelName)>;
    
    /**
     * @brief Callback when user wants to go back to server list
     */
    using BackCallback = std::function<void()>;
    
    ChannelList(int x, int y, int width, int height,
                ServerManager* serverManager,
                UserDatabase* userDatabase);
    ~ChannelList();
    
    /**
     * @brief Set the current server to display
     */
    void setServer(uint64_t serverId, uint64_t currentUserId);
    
    /**
     * @brief Refresh channel and member lists
     */
    void refresh();
    
    /**
     * @brief Set callback for channel selection
     */
    void setOnChannelSelected(ChannelSelectedCallback callback);
    
    /**
     * @brief Set callback for back button
     */
    void setOnBackClicked(BackCallback callback);
    
    /**
     * @brief Apply dark/light theme
     */
    void applyTheme(bool isDarkMode);
    
    /**
     * @brief Get currently selected channel ID
     */
    uint64_t getSelectedChannelId() const;
    
private:
    ServerManager* serverManager;
    UserDatabase* userDatabase;
    
    uint64_t currentServerId;
    uint64_t currentUserId;
    bool isOwner;
    
    ChannelSelectedCallback onChannelSelectedCallback;
    BackCallback onBackCallback;
    
    // UI Components
    Fl_Button* backButton;
    Fl_Box* serverNameLabel;
    Fl_Button* serverOptionsButton;
    Fl_Box* channelsHeader;
    Fl_Hold_Browser* channelList;
    Fl_Button* addChannelButton;
    Fl_Box* membersHeader;
    Fl_Hold_Browser* memberList;
    
    // Cached data
    std::vector<Models::Channel> cachedChannels;
    std::vector<uint64_t> cachedMemberIds;
    uint64_t selectedChannelId;
    
    // Layout
    void setupLayout();
    
    // Update UI
    void updateChannelList();
    void updateMemberList();
    void updateOwnerControls();
    
    // Callbacks
    static void onBackClicked(Fl_Widget* widget, void* userdata);
    static void onChannelListSelected(Fl_Widget* widget, void* userdata);
    static void onAddChannelClicked(Fl_Widget* widget, void* userdata);
    static void onServerOptionsClicked(Fl_Widget* widget, void* userdata);
};

#endif // CHANNEL_LIST_H
