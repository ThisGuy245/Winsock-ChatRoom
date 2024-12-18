#include "PlayerDisplay.hpp"
#include <FL/fl_draw.H>
#include <algorithm>
#include <cstdio> // For printf debugging

// Constructor: Initializes the player display with a grey side panel
PlayerDisplay::PlayerDisplay(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H), sidePanel(nullptr), disp(nullptr), tbuff(nullptr) {
    begin();

    // Create the side panel (grey background)
    sidePanel = new Fl_Box(X, Y, W / 2, H - 40); // Adjusted width to half
    sidePanel->box(FL_FLAT_BOX);
    sidePanel->color(FL_DARK3); // Grey color
    sidePanel->label(nullptr);

    // Create the text display for player list
    disp = new Fl_Text_Display(X + W / 2, Y, W / 2, H - 40); // Adjusted position and width
    tbuff = new Fl_Text_Buffer(); // Text buffer
    disp->buffer(tbuff);
    tbuff->text("Players:\n");

    end();
}

// Destructor: Cleans up dynamically allocated boxes
PlayerDisplay::~PlayerDisplay() {
    delete sidePanel;
    delete disp;
    delete tbuff;
}

// Adds a player to the display
void PlayerDisplay::addPlayer(const std::string& username) {
    // Debugging output
    printf("Adding player: %s\n", username.c_str());

    // Append the username to the text buffer
    tbuff->append((username + "\n").c_str());

    redraw();
}

// Removes a player by name
void PlayerDisplay::removePlayer(const std::string& username) {
    // Debugging output
    printf("Removing player: %s\n", username.c_str());

    // Find and remove the username from the text buffer
    const char* text = tbuff->text();
    std::string bufferText(text);
    size_t pos = bufferText.find(username);
    if (pos != std::string::npos) {
        bufferText.erase(pos, username.length() + 1); // +1 to remove the newline character
        tbuff->text(bufferText.c_str());
    }

    redraw();
}

// Updates the layout of player boxes
void PlayerDisplay::updateLayout() {
    int yOffset = 10;

    redraw();
}