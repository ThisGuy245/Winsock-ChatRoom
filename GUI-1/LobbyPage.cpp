#include "LobbyPage.hpp"
#include <FL/Fl_Ask.H>
#include <FL/fl_draw.H>
#include "HomePage.hpp"
#include "MainWindow.h"
#include "SettingsWindow.hpp"
#include "AboutWindow.h"

// Layout constants
static const int HEADER_HEIGHT = 50;
static const int INPUT_BAR_HEIGHT = 60;
static const int MEMBER_PANEL_WIDTH = 220;
static const int PADDING = 10;

/**
 * @brief Constructor for LobbyPage class.
 * Creates a modern, full-screen Discord-like layout.
 */
LobbyPage::LobbyPage(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H)
    , client(nullptr)
    , server(nullptr)
    , settings(nullptr)
    , about(nullptr)
    , ipInput(nullptr)
    , portInput(nullptr)
    , currentPort(54000)
    , darkMode(false)
    , currentServerName("Server")
    , currentChannelName("general")
    , currentChannelId(0)
    , messageService(nullptr)
{
    begin();
    
    // =========================================================================
    // HEADER BAR - Top section with navigation and controls
    // =========================================================================
    headerBar = new Fl_Group(X, Y, W, HEADER_HEIGHT);
    headerBar->box(FL_FLAT_BOX);
    headerBar->color(fl_rgb_color(47, 49, 54));  // Discord dark header
    headerBar->begin();
    
    // Back button (left side)
    backButton = new Fl_Button(X + PADDING, Y + 10, 70, 30, "@< Back");
    backButton->box(FL_FLAT_BOX);
    backButton->color(fl_rgb_color(88, 101, 242));  // Discord blurple
    backButton->labelcolor(FL_WHITE);
    backButton->callback(backButtonCallback, this);
    
    // Server/Channel name (center-left)
    serverNameLabel = new Fl_Box(X + 90, Y + 5, 200, 20);
    serverNameLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    serverNameLabel->labelcolor(FL_WHITE);
    serverNameLabel->labelsize(16);
    serverNameLabel->labelfont(FL_BOLD);
    serverNameLabel->label("Server Name");
    
    channelNameLabel = new Fl_Box(X + 90, Y + 25, 200, 20);
    channelNameLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    channelNameLabel->labelcolor(fl_rgb_color(142, 146, 151));  // Muted text
    channelNameLabel->labelsize(12);
    channelNameLabel->label("# general");
    
    // Settings button (right side)
    settingsButton = new Fl_Button(X + W - 90, Y + 10, 35, 30, "@menu");
    settingsButton->box(FL_FLAT_BOX);
    settingsButton->color(fl_rgb_color(64, 68, 75));
    settingsButton->labelcolor(FL_WHITE);
    settingsButton->tooltip("Settings");
    settingsButton->callback(settingsButtonCallback, this);
    
    // About/Info button (right side)
    aboutButton = new Fl_Button(X + W - 50, Y + 10, 35, 30, "?");
    aboutButton->box(FL_FLAT_BOX);
    aboutButton->color(fl_rgb_color(64, 68, 75));
    aboutButton->labelcolor(FL_WHITE);
    aboutButton->labelsize(16);
    aboutButton->tooltip("About");
    aboutButton->callback(aboutButtonCallback, this);
    
    headerBar->end();
    
    // =========================================================================
    // MAIN CONTENT AREA
    // =========================================================================
    int mainY = Y + HEADER_HEIGHT;
    int mainH = H - HEADER_HEIGHT - INPUT_BAR_HEIGHT;
    int chatW = W - MEMBER_PANEL_WIDTH;
    
    mainArea = new Fl_Group(X, mainY, W, mainH);
    mainArea->begin();
    
    // -------------------------------------------------------------------------
    // CHAT AREA (left/center)
    // -------------------------------------------------------------------------
    chatArea = new Fl_Group(X, mainY, chatW, mainH);
    chatArea->box(FL_FLAT_BOX);
    chatArea->color(fl_rgb_color(54, 57, 63));  // Discord chat bg
    chatArea->begin();
    
    // Chat display with scroll
    chatDisplay = new Fl_Text_Display(X + PADDING, mainY + PADDING, 
                                       chatW - 2 * PADDING, mainH - 2 * PADDING);
    chatBuffer = new Fl_Text_Buffer();
    chatDisplay->buffer(chatBuffer);
    chatDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    chatDisplay->box(FL_FLAT_BOX);
    chatDisplay->color(fl_rgb_color(54, 57, 63));
    chatDisplay->textcolor(fl_rgb_color(220, 221, 222));  // Light text
    chatDisplay->textsize(14);
    chatDisplay->scrollbar_width(12);
    
    chatArea->end();
    chatArea->resizable(chatDisplay);
    
    // -------------------------------------------------------------------------
    // MEMBER PANEL (right side)
    // -------------------------------------------------------------------------
    memberPanel = new Fl_Group(X + chatW, mainY, MEMBER_PANEL_WIDTH, mainH);
    memberPanel->box(FL_FLAT_BOX);
    memberPanel->color(fl_rgb_color(47, 49, 54));  // Darker side panel
    memberPanel->begin();
    
    // Member header
    memberHeaderLabel = new Fl_Box(X + chatW + PADDING, mainY + PADDING, 
                                    MEMBER_PANEL_WIDTH - 2 * PADDING, 25);
    memberHeaderLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    memberHeaderLabel->labelcolor(fl_rgb_color(142, 146, 151));
    memberHeaderLabel->labelsize(12);
    memberHeaderLabel->labelfont(FL_BOLD);
    memberHeaderLabel->label("ONLINE MEMBERS");
    
    // Player display
    playerDisplay = new PlayerDisplay(X + chatW + PADDING, mainY + 40, 
                                       MEMBER_PANEL_WIDTH - 2 * PADDING, mainH - 50);
    
    memberPanel->end();
    
    mainArea->end();
    mainArea->resizable(chatArea);
    
    // =========================================================================
    // INPUT BAR - Bottom message input area
    // =========================================================================
    int inputY = Y + H - INPUT_BAR_HEIGHT;
    
    inputBar = new Fl_Group(X, inputY, W, INPUT_BAR_HEIGHT);
    inputBar->box(FL_FLAT_BOX);
    inputBar->color(fl_rgb_color(64, 68, 75));  // Slightly lighter input area
    inputBar->begin();
    
    // Message input field (spans most of width)
    int inputW = W - MEMBER_PANEL_WIDTH - 100 - 3 * PADDING;
    messageInput = new Fl_Input(X + PADDING, inputY + 15, inputW, 30);
    messageInput->box(FL_FLAT_BOX);
    messageInput->color(fl_rgb_color(64, 68, 75));
    messageInput->textcolor(fl_rgb_color(220, 221, 222));
    messageInput->textsize(14);
    messageInput->when(FL_WHEN_ENTER_KEY_ALWAYS);
    messageInput->callback(messageInputCallback, this);
    
    // Placeholder text behavior - we'll use tooltip for now
    messageInput->tooltip("Type a message...");
    
    // Send button
    sendButton = new Fl_Button(X + PADDING + inputW + PADDING, inputY + 15, 80, 30, "Send");
    sendButton->box(FL_FLAT_BOX);
    sendButton->color(fl_rgb_color(88, 101, 242));  // Discord blurple
    sendButton->labelcolor(FL_WHITE);
    sendButton->callback(sendButtonCallback, this);
    
    inputBar->end();
    
    // =========================================================================
    // HIDDEN INPUTS (for network configuration)
    // =========================================================================
    ipInput = new Fl_Input(0, 0, 1, 1);
    ipInput->hide();
    ipInput->value("127.0.0.1");
    
    portInput = new Fl_Input(0, 0, 1, 1);
    portInput->hide();
    portInput->value("54000");
    
    end();
    
    // Set resizable behavior
    resizable(mainArea);
}

/**
 * @brief Destructor - cleanup network resources
 */
LobbyPage::~LobbyPage() {
    delete client;
    delete server;
}

/**
 * @brief Update server name displayed in header
 */
void LobbyPage::setServerName(const std::string& name) {
    currentServerName = name;
    if (serverNameLabel) {
        // Need to store the string since label() doesn't copy
        static char serverNameBuf[256];
        strncpy_s(serverNameBuf, sizeof(serverNameBuf), name.c_str(), _TRUNCATE);
        serverNameLabel->label(serverNameBuf);
        serverNameLabel->redraw();
    }
}

/**
 * @brief Update channel name displayed in header
 */
void LobbyPage::setChannelName(const std::string& name) {
    currentChannelName = name;
    if (channelNameLabel) {
        static char channelNameBuf[256];
        snprintf(channelNameBuf, sizeof(channelNameBuf), "# %s", name.c_str());
        channelNameLabel->label(channelNameBuf);
        channelNameLabel->redraw();
    }
}

/**
 * @brief Resize all widgets when window size changes
 */
void LobbyPage::resizeWidgets(int X, int Y, int W, int H) {
    // Header bar
    headerBar->resize(X, Y, W, HEADER_HEIGHT);
    backButton->resize(X + PADDING, Y + 10, 70, 30);
    serverNameLabel->resize(X + 90, Y + 5, W - 250, 20);
    channelNameLabel->resize(X + 90, Y + 25, W - 250, 20);
    settingsButton->resize(X + W - 90, Y + 10, 35, 30);
    aboutButton->resize(X + W - 50, Y + 10, 35, 30);
    
    // Main area
    int mainY = Y + HEADER_HEIGHT;
    int mainH = H - HEADER_HEIGHT - INPUT_BAR_HEIGHT;
    int chatW = W - MEMBER_PANEL_WIDTH;
    
    mainArea->resize(X, mainY, W, mainH);
    
    // Chat area
    chatArea->resize(X, mainY, chatW, mainH);
    chatDisplay->resize(X + PADDING, mainY + PADDING, chatW - 2 * PADDING, mainH - 2 * PADDING);
    
    // Member panel
    memberPanel->resize(X + chatW, mainY, MEMBER_PANEL_WIDTH, mainH);
    memberHeaderLabel->resize(X + chatW + PADDING, mainY + PADDING, MEMBER_PANEL_WIDTH - 2 * PADDING, 25);
    playerDisplay->resize(X + chatW + PADDING, mainY + 40, MEMBER_PANEL_WIDTH - 2 * PADDING, mainH - 50);
    
    // Input bar
    int inputY = Y + H - INPUT_BAR_HEIGHT;
    int inputW = W - MEMBER_PANEL_WIDTH - 100 - 3 * PADDING;
    
    inputBar->resize(X, inputY, W, INPUT_BAR_HEIGHT);
    messageInput->resize(X + PADDING, inputY + 15, inputW, 30);
    sendButton->resize(X + PADDING + inputW + PADDING, inputY + 15, 80, 30);
}

/**
 * @brief Apply dark or light theme to all components
 */
void LobbyPage::applyTheme(bool isDarkMode) {
    darkMode = isDarkMode;
    updateColors();
}

/**
 * @brief Update colors based on current theme
 */
void LobbyPage::updateColors() {
    if (darkMode) {
        // Dark theme (Discord-like)
        Fl_Color headerBg = fl_rgb_color(47, 49, 54);
        Fl_Color chatBg = fl_rgb_color(54, 57, 63);
        Fl_Color sidebarBg = fl_rgb_color(47, 49, 54);
        Fl_Color inputBg = fl_rgb_color(64, 68, 75);
        Fl_Color textColor = fl_rgb_color(220, 221, 222);
        Fl_Color mutedText = fl_rgb_color(142, 146, 151);
        Fl_Color accentColor = fl_rgb_color(88, 101, 242);
        
        headerBar->color(headerBg);
        chatArea->color(chatBg);
        chatDisplay->color(chatBg);
        chatDisplay->textcolor(textColor);
        memberPanel->color(sidebarBg);
        inputBar->color(inputBg);
        messageInput->color(inputBg);
        messageInput->textcolor(textColor);
        
        serverNameLabel->labelcolor(FL_WHITE);
        channelNameLabel->labelcolor(mutedText);
        memberHeaderLabel->labelcolor(mutedText);
        
        backButton->color(accentColor);
        backButton->labelcolor(FL_WHITE);
        sendButton->color(accentColor);
        sendButton->labelcolor(FL_WHITE);
        settingsButton->color(inputBg);
        settingsButton->labelcolor(FL_WHITE);
        aboutButton->color(inputBg);
        aboutButton->labelcolor(FL_WHITE);
    } else {
        // Light theme
        Fl_Color headerBg = fl_rgb_color(240, 240, 240);
        Fl_Color chatBg = FL_WHITE;
        Fl_Color sidebarBg = fl_rgb_color(245, 245, 245);
        Fl_Color inputBg = fl_rgb_color(235, 235, 235);
        Fl_Color textColor = fl_rgb_color(30, 30, 30);
        Fl_Color mutedText = fl_rgb_color(100, 100, 100);
        Fl_Color accentColor = fl_rgb_color(88, 101, 242);
        
        headerBar->color(headerBg);
        chatArea->color(chatBg);
        chatDisplay->color(chatBg);
        chatDisplay->textcolor(textColor);
        memberPanel->color(sidebarBg);
        inputBar->color(inputBg);
        messageInput->color(FL_WHITE);
        messageInput->textcolor(textColor);
        
        serverNameLabel->labelcolor(textColor);
        channelNameLabel->labelcolor(mutedText);
        memberHeaderLabel->labelcolor(mutedText);
        
        backButton->color(accentColor);
        backButton->labelcolor(FL_WHITE);
        sendButton->color(accentColor);
        sendButton->labelcolor(FL_WHITE);
        settingsButton->color(fl_rgb_color(200, 200, 200));
        settingsButton->labelcolor(textColor);
        aboutButton->color(fl_rgb_color(200, 200, 200));
        aboutButton->labelcolor(textColor);
    }
    
    redraw();
}

// =============================================================================
// NETWORKING METHODS
// =============================================================================

void LobbyPage::hostServer(const std::string& ip, const std::string& username) {
    cleanupSession();
    this->username = username;
    server = new ServerSocket(12345, playerDisplay, "config.xml");
    chatBuffer->append("Server has been created\n");

    try {
        client = new ClientSocket(ip, 12345, username, playerDisplay, "config.xml", nullptr);
    }
    catch (const std::exception& e) {
        chatBuffer->append(("[ERROR]: Failed to initialize client: " + std::string(e.what()) + "\n").c_str());
    }
}

void LobbyPage::joinServer(const std::string& ip, const std::string& username) {
    cleanupSession();
    this->username = username;
    client = new ClientSocket(ip, 12345, username, playerDisplay, "config.xml", nullptr);
}

void LobbyPage::hostServer() {
    cleanupSession();
    
    int port = 54000;
    if (portInput && portInput->value()) {
        port = std::atoi(portInput->value());
        if (port <= 0 || port > 65535) {
            port = 54000;
        }
    }
    currentPort = static_cast<uint16_t>(port);
    
    server = new ServerSocket(port, playerDisplay, "config.xml");
    chatBuffer->append("Server has been created\n");
    
    try {
        std::string ip = "127.0.0.1";
        client = new ClientSocket(ip, port, username, playerDisplay, "config.xml", nullptr);
        printf("[LOBBY] Hosting on port %d as '%s'\n", port, username.c_str());
    }
    catch (const std::exception& e) {
        chatBuffer->append(("[ERROR]: Failed to initialize client: " + std::string(e.what()) + "\n").c_str());
    }
}

void LobbyPage::joinServer() {
    cleanupSession();
    
    std::string ip = "127.0.0.1";
    int port = 54000;
    
    if (ipInput && ipInput->value()) {
        ip = ipInput->value();
    }
    if (portInput && portInput->value()) {
        port = std::atoi(portInput->value());
        if (port <= 0 || port > 65535) {
            port = 54000;
        }
    }
    currentPort = static_cast<uint16_t>(port);
    
    try {
        client = new ClientSocket(ip, port, username, playerDisplay, "config.xml", nullptr);
        printf("[LOBBY] Joining %s:%d as '%s'\n", ip.c_str(), port, username.c_str());
    }
    catch (const std::exception& e) {
        chatBuffer->append(("[ERROR]: Failed to connect: " + std::string(e.what()) + "\n").c_str());
    }
}

void LobbyPage::disconnectAndReset() {
    if (client) {
        delete client;
        client = nullptr;
    }
    if (server) {
        delete server;
        server = nullptr;
    }
    cleanupSession();
    printf("[LOBBY] Disconnected and reset\n");
}

void LobbyPage::clientLeft(const std::string& clientUsername) {
    chatBuffer->append(("[SERVER]: " + clientUsername + " has left the server\n").c_str());
}

void LobbyPage::cleanupSession() {
    if (chatBuffer) {
        chatBuffer->text("");
    }
    if (playerDisplay) {
        playerDisplay->clearPlayers();
    }
    if (chatDisplay) {
        chatDisplay->redraw();
    }
    currentChannelId = 0;
}

/**
 * @brief Load message history for a specific channel
 */
void LobbyPage::loadChannelHistory(uint64_t channelId, MessageService* service) {
    if (!chatBuffer || !service) {
        return;
    }
    
    // Store references
    messageService = service;
    currentChannelId = channelId;
    
    // Reload from file to get latest messages from other users
    service->ReloadFromFile();
    
    // Clear current chat display
    chatBuffer->text("");
    
    // Load messages for this channel
    auto messages = service->GetChannelMessages(channelId);
    
    printf("[LOBBY] Loading %zu messages for channel %llu\n", messages.size(), channelId);
    
    for (const auto& msg : messages) {
        // Messages are stored as they were received from the server
        // Just display them directly
        chatBuffer->append((msg.content + "\n").c_str());
    }
    
    // Scroll to bottom
    if (chatDisplay) {
        chatDisplay->scroll(chatBuffer->count_lines(0, chatBuffer->length()), 0);
        chatDisplay->redraw();
    }
}

/**
 * @brief Save a user message to the channel history
 */
void LobbyPage::saveMessageToHistory(const std::string& senderName, const std::string& content) {
    if (!messageService || currentChannelId == 0) {
        return;
    }
    
    // The content already includes "username: message" format from the server
    // Store it directly
    messageService->AddMessage(currentChannelId, 0, senderName, content, Models::MessageType::Text);
}

/**
 * @brief Save a system message to the channel history
 */
void LobbyPage::saveSystemMessageToHistory(const std::string& content) {
    if (!messageService || currentChannelId == 0) {
        return;
    }
    
    messageService->AddSystemMessage(currentChannelId, content);
}

void LobbyPage::sendMessage(const std::string& message) {
    if (client && !message.empty()) {
        client->send(message);
    }
}

void LobbyPage::receiveMessages() {
    std::string message;
    if (client && client->receive(message)) {
        chatBuffer->append((message + "\n").c_str());
        
        // Save to channel history
        if (messageService && currentChannelId != 0) {
            // Check if it's a system message (join/leave)
            if (message.find("[SERVER]:") != std::string::npos) {
                saveSystemMessageToHistory(message);
            } else {
                // Regular user message - store the whole formatted message
                saveMessageToHistory(username, message);
            }
        }
        
        // Auto-scroll to bottom
        chatDisplay->scroll(chatBuffer->count_lines(0, chatBuffer->length()), 0);
    }
    if (server) {
        server->handleClientConnections();
    }
}

void LobbyPage::changeUsername(const std::string& newUsername) {
    try {
        if (client) {
            client->changeUsername(newUsername);
        }
    }
    catch (const std::exception& e) {
        if (chatBuffer) {
            chatBuffer->append(("[ERROR]: Failed to change username: " + std::string(e.what()) + "\n").c_str());
        }
    }
}

void LobbyPage::Update() {
    receiveMessages();
}

// =============================================================================
// CALLBACKS
// =============================================================================

void LobbyPage::backButtonCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* page = static_cast<LobbyPage*>(userdata);
    
    // Disconnect first
    if (page->client || page->server) {
        page->disconnectAndReset();
    }
    
    // Call the back callback if set
    if (page->onBackClicked) {
        page->onBackClicked();
    } else {
        // Fallback: try to navigate via MainWindow
        MainWindow* mainWindow = dynamic_cast<MainWindow*>(page->parent());
        if (mainWindow) {
            mainWindow->switchToServerBrowser();
        }
    }
}

void LobbyPage::settingsButtonCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* page = static_cast<LobbyPage*>(userdata);
    MainWindow* mainWindow = dynamic_cast<MainWindow*>(page->parent());
    
    if (page->onSettingsClicked) {
        page->onSettingsClicked();
    } else if (!page->settings) {
        page->settings = new SettingsWindow(400, 300, "Settings", mainWindow, page);
        page->settings->show();
    } else {
        page->settings->show();
    }
}

void LobbyPage::aboutButtonCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* page = static_cast<LobbyPage*>(userdata);
    
    if (!page->about) {
        page->about = new AboutWindow(450, 350, "About");
        page->about->show();
    } else {
        page->about->show();
    }
}

void LobbyPage::sendButtonCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* page = static_cast<LobbyPage*>(userdata);
    if (page && page->messageInput) {
        const std::string message = page->messageInput->value();
        if (!message.empty()) {
            page->sendMessage(message);
            page->messageInput->value("");
        }
    }
}

void LobbyPage::messageInputCallback(Fl_Widget* widget, void* userdata) {
    LobbyPage* page = static_cast<LobbyPage*>(userdata);
    if (page) {
        Fl_Input* input = static_cast<Fl_Input*>(widget);
        const std::string message = input->value();
        if (!message.empty()) {
            page->sendMessage(message);
            input->value("");
        }
    }
}
