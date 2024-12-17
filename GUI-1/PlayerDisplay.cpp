#include "PlayerDisplay.hpp"
#include <FL/fl_draw.H>
#include <algorithm>

// Constructor: Initializes the scrollable player display area
PlayerDisplay::PlayerDisplay(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {
    begin();

    // Create the scrollable area
    scrollArea = new Fl_Scroll(X, Y, W, H);
    scrollArea->type(FL_VERTICAL);  // Enable vertical scrolling

    end();
    resizable(scrollArea);  // Allow resizing of scrollArea
}

// Destructor: Cleans up dynamically allocated boxes
PlayerDisplay::~PlayerDisplay() {
    clear();           // Remove all player boxes
    delete scrollArea; // Delete the scroll area
}

// Clear function: Removes all player widgets
void PlayerDisplay::clear() {
    for (auto playerBox : playerBoxes) {
        scrollArea->remove(playerBox);  // Remove from scroll area
        delete playerBox;               // Free memory
    }
    playerBoxes.clear();
    redraw();  // Refresh the widget
}

// Adds a single player to the display
void PlayerDisplay::addPlayer(const std::string& username) {
    // Create a new box for the player's name
    Fl_Box* playerBox = new Fl_Box(
        scrollArea->x() + 10,              // X position relative to scrollArea
        scrollArea->y() + 10 + (20 * playerBoxes.size()),  // Y position (next row)
        scrollArea->w() - 20,              // Width
        20,                                // Height
        username.c_str());                 // Label (player's name)

    playerBox->box(FL_NO_BOX);
    playerBox->labelfont(FL_HELVETICA);
    playerBox->labelsize(14);

    scrollArea->add(playerBox);   // Add to scroll area
    playerBoxes.push_back(playerBox);  // Add to internal list
    redraw();
}

// Removes a player by name
void PlayerDisplay::removePlayer(const std::string& playerName) {
    auto it = std::find_if(playerBoxes.begin(), playerBoxes.end(),
        [&playerName](Fl_Box* box) {
            return playerName == box->label();
        });

    if (it != playerBoxes.end()) {
        Fl_Box* boxToRemove = *it;
        scrollArea->remove(boxToRemove);
        playerBoxes.erase(it);
        delete boxToRemove;
        updateLayout();  // Adjust layout after removal
    }
}

// Updates the layout after player removal
void PlayerDisplay::updateLayout() {
    int yOffset = 10;
    for (auto box : playerBoxes) {
        box->resize(scrollArea->x() + 10, scrollArea->y() + yOffset,
            scrollArea->w() - 20, 20);
        yOffset += 20;
    }
    redraw();
}

