/**
 * @file ChannelList.cpp
 * @brief Implementation of the channel list sidebar
 */

#include "ChannelList.h"
#include "ServerManager.h"
#include "UserDatabase.h"
#include <FL/fl_ask.H>

// Color constants
namespace {
    const Fl_Color DARK_BG = fl_rgb_color(47, 49, 54);
    const Fl_Color DARK_ITEM = fl_rgb_color(54, 57, 63);
    const Fl_Color DARK_HEADER = fl_rgb_color(32, 34, 37);
    const Fl_Color DARK_TEXT = FL_WHITE;
    const Fl_Color DARK_MUTED = fl_rgb_color(150, 150, 150);
    const Fl_Color LIGHT_BG = fl_rgb_color(242, 243, 245);
    const Fl_Color LIGHT_TEXT = FL_BLACK;
    const Fl_Color ACCENT_COLOR = fl_rgb_color(88, 101, 242);
    const Fl_Color ONLINE_COLOR = fl_rgb_color(87, 242, 135);
}

ChannelList::ChannelList(int x, int y, int width, int height,
                         ServerManager* serverMgr,
                         UserDatabase* userDb)
    : Fl_Group(x, y, width, height)
    , serverManager(serverMgr)
    , userDatabase(userDb)
    , currentServerId(0)
    , currentUserId(0)
    , isOwner(false)
    , selectedChannelId(0)
    , backButton(nullptr)
    , serverNameLabel(nullptr)
    , serverOptionsButton(nullptr)
    , channelsHeader(nullptr)
    , channelList(nullptr)
    , addChannelButton(nullptr)
    , membersHeader(nullptr)
    , memberList(nullptr) {
    
    setupLayout();
    end();
}

ChannelList::~ChannelList() {
    // FLTK handles cleanup
}

void ChannelList::setupLayout() {
    int margin = 8;
    int buttonHeight = 30;
    int headerHeight = 20;
    int currentY = y() + margin;
    
    // Back button
    backButton = new Fl_Button(x() + margin, currentY, 60, buttonHeight, "< Back");
    backButton->box(FL_FLAT_BOX);
    backButton->labelsize(11);
    backButton->callback(onBackClicked, this);
    currentY += buttonHeight + margin;
    
    // Server name with options button
    serverNameLabel = new Fl_Box(x() + margin, currentY, w() - 60, 30, "Server Name");
    serverNameLabel->labelsize(14);
    serverNameLabel->labelfont(FL_BOLD);
    serverNameLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    serverOptionsButton = new Fl_Button(x() + w() - 45, currentY, 35, 30, "...");
    serverOptionsButton->box(FL_FLAT_BOX);
    serverOptionsButton->labelsize(14);
    serverOptionsButton->callback(onServerOptionsClicked, this);
    currentY += 35 + margin;
    
    // Channels header
    channelsHeader = new Fl_Box(x() + margin, currentY, w() - 2 * margin, headerHeight, "TEXT CHANNELS");
    channelsHeader->labelsize(10);
    channelsHeader->labelfont(FL_BOLD);
    channelsHeader->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    currentY += headerHeight;
    
    // Calculate heights for lists (split remaining space)
    int remainingHeight = h() - (currentY - y()) - margin;
    int channelListHeight = static_cast<int>(remainingHeight * 0.5);  // 50% for channels
    int memberListHeight = static_cast<int>(remainingHeight * 0.35);  // 35% for members
    
    // Channel list
    channelList = new Fl_Hold_Browser(x() + margin, currentY, w() - 2 * margin, channelListHeight);
    channelList->textsize(12);
    channelList->callback(onChannelListSelected, this);
    currentY += channelListHeight;
    
    // Add channel button (for owners)
    addChannelButton = new Fl_Button(x() + margin, currentY, w() - 2 * margin, buttonHeight, "+ Add Channel");
    addChannelButton->box(FL_FLAT_BOX);
    addChannelButton->labelsize(11);
    addChannelButton->callback(onAddChannelClicked, this);
    addChannelButton->hide();  // Only shown for owners
    currentY += buttonHeight + margin;
    
    // Members header
    membersHeader = new Fl_Box(x() + margin, currentY, w() - 2 * margin, headerHeight, "MEMBERS");
    membersHeader->labelsize(10);
    membersHeader->labelfont(FL_BOLD);
    membersHeader->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    currentY += headerHeight;
    
    // Member list
    memberList = new Fl_Hold_Browser(x() + margin, currentY, w() - 2 * margin, memberListHeight);
    memberList->textsize(12);
}

void ChannelList::setServer(uint64_t serverId, uint64_t userId) {
    currentServerId = serverId;
    currentUserId = userId;
    selectedChannelId = 0;
    
    // Check if user is owner
    isOwner = serverManager->IsServerOwner(serverId, userId);
    
    refresh();
}

void ChannelList::refresh() {
    if (currentServerId == 0) {
        return;
    }
    
    // Get server info
    Models::ChatServer server;
    if (!serverManager->GetServer(currentServerId, server)) {
        return;
    }
    
    serverNameLabel->copy_label(server.serverName.c_str());
    
    updateChannelList();
    updateMemberList();
    updateOwnerControls();
}

void ChannelList::updateChannelList() {
    channelList->clear();
    cachedChannels = serverManager->GetServerChannels(currentServerId);
    
    for (const Models::Channel& channel : cachedChannels) {
        std::string displayName = "# " + channel.channelName;
        channelList->add(displayName.c_str());
    }
    
    // Auto-select first channel if none selected
    if (selectedChannelId == 0 && !cachedChannels.empty()) {
        selectedChannelId = cachedChannels[0].channelId;
        channelList->value(1);
        
        if (onChannelSelectedCallback) {
            onChannelSelectedCallback(cachedChannels[0].channelId, cachedChannels[0].channelName);
        }
    }
}

void ChannelList::updateMemberList() {
    memberList->clear();
    cachedMemberIds = serverManager->GetServerMembers(currentServerId);
    
    // Get server to check owner
    Models::ChatServer server;
    serverManager->GetServer(currentServerId, server);
    
    for (uint64_t memberId : cachedMemberIds) {
        Models::User user;
        if (userDatabase->GetUserById(memberId, user)) {
            std::string displayName = user.username;
            
            // Mark owner
            if (memberId == server.ownerId) {
                displayName = user.username + " (Owner)";
            }
            
            // Mark online status
            if (user.isOnline) {
                displayName = "@C2@." + displayName;  // Green dot prefix
            }
            
            memberList->add(displayName.c_str());
        }
    }
}

void ChannelList::updateOwnerControls() {
    if (isOwner) {
        addChannelButton->show();
    } else {
        addChannelButton->hide();
    }
}

void ChannelList::setOnChannelSelected(ChannelSelectedCallback callback) {
    onChannelSelectedCallback = callback;
}

void ChannelList::setOnBackClicked(BackCallback callback) {
    onBackCallback = callback;
}

void ChannelList::applyTheme(bool isDarkMode) {
    if (isDarkMode) {
        color(DARK_BG);
        
        backButton->color(DARK_ITEM);
        backButton->labelcolor(DARK_TEXT);
        
        serverNameLabel->labelcolor(DARK_TEXT);
        serverOptionsButton->color(DARK_ITEM);
        serverOptionsButton->labelcolor(DARK_TEXT);
        
        channelsHeader->labelcolor(DARK_MUTED);
        channelList->color(DARK_BG);
        channelList->textcolor(DARK_TEXT);
        channelList->selection_color(ACCENT_COLOR);
        
        addChannelButton->color(DARK_ITEM);
        addChannelButton->labelcolor(DARK_TEXT);
        
        membersHeader->labelcolor(DARK_MUTED);
        memberList->color(DARK_BG);
        memberList->textcolor(DARK_TEXT);
    } else {
        color(LIGHT_BG);
        
        backButton->color(FL_WHITE);
        backButton->labelcolor(LIGHT_TEXT);
        
        serverNameLabel->labelcolor(LIGHT_TEXT);
        serverOptionsButton->color(FL_WHITE);
        serverOptionsButton->labelcolor(LIGHT_TEXT);
        
        channelsHeader->labelcolor(fl_rgb_color(100, 100, 100));
        channelList->color(FL_WHITE);
        channelList->textcolor(LIGHT_TEXT);
        channelList->selection_color(ACCENT_COLOR);
        
        addChannelButton->color(FL_WHITE);
        addChannelButton->labelcolor(LIGHT_TEXT);
        
        membersHeader->labelcolor(fl_rgb_color(100, 100, 100));
        memberList->color(FL_WHITE);
        memberList->textcolor(LIGHT_TEXT);
    }
    
    redraw();
}

uint64_t ChannelList::getSelectedChannelId() const {
    return selectedChannelId;
}

// Static callbacks

void ChannelList::onBackClicked(Fl_Widget* widget, void* userdata) {
    ChannelList* list = static_cast<ChannelList*>(userdata);
    
    if (list->onBackCallback) {
        list->onBackCallback();
    }
}

void ChannelList::onChannelListSelected(Fl_Widget* widget, void* userdata) {
    ChannelList* list = static_cast<ChannelList*>(userdata);
    
    int selected = list->channelList->value();
    if (selected <= 0 || selected > static_cast<int>(list->cachedChannels.size())) {
        return;
    }
    
    const Models::Channel& channel = list->cachedChannels[selected - 1];
    list->selectedChannelId = channel.channelId;
    
    if (list->onChannelSelectedCallback) {
        list->onChannelSelectedCallback(channel.channelId, channel.channelName);
    }
}

void ChannelList::onAddChannelClicked(Fl_Widget* widget, void* userdata) {
    ChannelList* list = static_cast<ChannelList*>(userdata);
    
    if (!list->isOwner) {
        fl_alert("Only the server owner can create channels.");
        return;
    }
    
    const char* channelName = fl_input("Enter channel name:", "general");
    
    if (channelName && strlen(channelName) > 0) {
        Models::Channel newChannel;
        Protocol::ErrorCode result = list->serverManager->CreateChannel(
            list->currentServerId,
            channelName,
            list->currentUserId,
            newChannel
        );
        
        if (result == Protocol::ErrorCode::None) {
            list->refresh();
            
            // Select the new channel
            list->selectedChannelId = newChannel.channelId;
            if (list->onChannelSelectedCallback) {
                list->onChannelSelectedCallback(newChannel.channelId, newChannel.channelName);
            }
        } else {
            fl_alert("Failed to create channel: %s",
                     Protocol::ErrorCodeToMessage(result));
        }
    }
}

void ChannelList::onServerOptionsClicked(Fl_Widget* widget, void* userdata) {
    ChannelList* list = static_cast<ChannelList*>(userdata);
    
    if (!list->isOwner) {
        // Non-owner options
        int choice = fl_choice("Server Options",
                               "Cancel", "Leave Server", nullptr);
        
        if (choice == 1) {
            int confirm = fl_choice("Are you sure you want to leave this server?",
                                    "Cancel", "Leave", nullptr);
            if (confirm == 1) {
                Protocol::ErrorCode result = list->serverManager->LeaveServer(
                    list->currentServerId, list->currentUserId
                );
                
                if (result == Protocol::ErrorCode::None) {
                    fl_message("Left the server.");
                    if (list->onBackCallback) {
                        list->onBackCallback();
                    }
                }
            }
        }
    } else {
        // Owner options
        int choice = fl_choice("Server Options (Owner)",
                               "Cancel", "Rename Server", "Delete Server");
        
        if (choice == 1) {
            // Rename
            const char* newName = fl_input("Enter new server name:");
            if (newName && strlen(newName) > 0) {
                Protocol::ErrorCode result = list->serverManager->RenameServer(
                    list->currentServerId, newName, list->currentUserId
                );
                
                if (result == Protocol::ErrorCode::None) {
                    list->refresh();
                } else {
                    fl_alert("Failed to rename: %s",
                             Protocol::ErrorCodeToMessage(result));
                }
            }
        } else if (choice == 2) {
            // Delete
            int confirm = fl_choice("Are you SURE you want to DELETE this server?\nThis cannot be undone!",
                                    "Cancel", "DELETE", nullptr);
            if (confirm == 1) {
                Protocol::ErrorCode result = list->serverManager->DeleteServer(
                    list->currentServerId, list->currentUserId
                );
                
                if (result == Protocol::ErrorCode::None) {
                    fl_message("Server deleted.");
                    if (list->onBackCallback) {
                        list->onBackCallback();
                    }
                }
            }
        }
    }
}
