#include "PlayerDisplay.hpp"
#include <FL/fl_draw.H>
#include <algorithm>

// Constructor: Initializes the scrollable player display area
PlayerDisplay::PlayerDisplay(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {
    begin();

    // Create the scrollable area
    scrollArea = new Fl_Scroll(X, Y + 20, W, H - 20);  // Adjust scroll area to make room for the "Players:" label
    scrollArea->type(FL_VERTICAL);  // Enable vertical scrolling

    // Create a static label for "Players:"
    playersLabel = new Fl_Box(X + 10, Y, W - 20, 20, "Players:");
    playersLabel->labelfont(FL_HELVETICA_BOLD);
    playersLabel->labelsize(16);
    playersLabel->labelcolor(FL_WHITE);

    end();
    resizable(scrollArea);  // Allow resizing of scrollArea
}

// Destructor: Cleans up dynamically allocated boxes
PlayerDisplay::~PlayerDisplay() {
    clear();           // Remove all player boxes
    delete scrollArea; // Delete the scroll area
    delete playersLabel; // Delete the players label
}

// Clear function: Removes all player widgets
void PlayerDisplay::clear() {
    for (auto playerBox : playerBoxes) {
        scrollArea->remove(playerBox);  // Remove from scroll area
        delete playerBox;               // Free memory
    }
    playerBoxes.clear();
    playerStatus.clear();
    redraw();  // Refresh the widget
}

// Adds a player to the display
void PlayerDisplay::addPlayer(const std::string& username) {
    if (std::find_if(playerStatus.begin(), playerStatus.end(),
        [&username](const auto& status) { return status.first == username; }) != playerStatus.end()) {
        return;  // Player already exists
    }

    Fl_Box* playerBox = new Fl_Box(
        scrollArea->x() + 10,              // X position
        scrollArea->y() + 10 + (20 * playerBoxes.size()),  // Y position
        scrollArea->w() - 20,              // Width
        20,                                // Height
        username.c_str());                 // Player's name

    playerBox->box(FL_NO_BOX);
    playerBox->labelfont(FL_HELVETICA);
    playerBox->labelsize(14);

    scrollArea->add(playerBox);   // Add to the scrollable area
    playerBoxes.push_back(playerBox);  // Track the player box
    playerStatus.push_back({ username, true });  // Add player as connected
    redraw();
}

// Removes a player by name
void PlayerDisplay::removePlayer(const std::string& username) {
    auto it = std::find_if(playerStatus.begin(), playerStatus.end(),
        [&username](const auto& status) { return status.first == username; });

    if (it != playerStatus.end()) {
        size_t index = std::distance(playerStatus.begin(), it);
        Fl_Box* boxToRemove = playerBoxes[index];

        scrollArea->remove(boxToRemove);
        playerBoxes.erase(playerBoxes.begin() + index);
        delete boxToRemove;

        playerStatus.erase(it);
        updateLayout();
    }
}


// Update the player's connection status
void PlayerDisplay::updatePlayerStatus(const std::string& username, bool isConnected) {
    for (size_t i = 0; i < playerStatus.size(); ++i) {
        if (playerStatus[i].first == username) {
            playerStatus[i].second = isConnected;

            // Optionally, change color based on connection status
            if (isConnected) {
                playerBoxes[i]->labelcolor(FL_GREEN);  // Green for connected
            }
            else {
                playerBoxes[i]->labelcolor(FL_RED);  // Red for disconnected
            }

            playerBoxes[i]->redraw();
            break;
        }
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

// Get the current list of players
std::vector<std::string> PlayerDisplay::getPlayers() {
    std::vector<std::string> players;
    for (const auto& player : playerStatus) {
        players.push_back(player.first); // Add player name
    }
    return players;
}
