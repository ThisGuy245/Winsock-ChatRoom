#include "PlayerDisplay.hpp"
#include <FL/fl_draw.H>
#include <algorithm>
#include <cstdio> // For printf debugging

// Constructor: Initializes the player display with a grey side panel
PlayerDisplay::PlayerDisplay(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H), sidePanel(nullptr) {
    begin();

    // Create the side panel (grey background)
    sidePanel = new Fl_Box(X, Y, W, H);
    sidePanel->box(FL_FLAT_BOX);
    sidePanel->color(FL_DARK3); // Grey color
    sidePanel->label(nullptr);

    end();
}

// Destructor: Cleans up dynamically allocated boxes
PlayerDisplay::~PlayerDisplay() {
    for (auto playerBox : playerBoxes) {
        remove(playerBox);
        delete playerBox;
    }
    playerBoxes.clear();
    delete sidePanel;
}

// Adds a player to the display
void PlayerDisplay::addPlayer(const std::string& username) {
    // Debugging output
    printf("Adding player: %s\n", username.c_str());

    // Check if the player already exists
    if (std::any_of(playerBoxes.begin(), playerBoxes.end(), [&](Fl_Box* box) {
        return box->label() && username == box->label();
        })) {
        printf("Player already exists: %s\n", username.c_str());
        return;
    }

    // Create a new player box
    Fl_Box* playerBox = new Fl_Box(x() + 10, y() + 10 + (20 * playerBoxes.size()),
        w() - 20, 20, username.c_str());
    playerBox->box(FL_FLAT_BOX);
    playerBox->labelfont(FL_HELVETICA);
    playerBox->labelsize(14);
    playerBox->labelcolor(FL_WHITE); // White text on grey background
    playerBox->color(FL_DARK2);      // Slightly lighter grey

    // Add the player box and update layout
    playerBoxes.push_back(playerBox);
    add(playerBox);
    updateLayout();
    redraw();
}

// Removes a player by name
void PlayerDisplay::removePlayer(const std::string& username) {
    auto it = std::find_if(playerBoxes.begin(), playerBoxes.end(), [&](Fl_Box* box) {
        return box->label() && username == box->label();
        });

    if (it != playerBoxes.end()) {
        Fl_Box* boxToRemove = *it;

        // Debugging output
        printf("Removing player: %s\n", username.c_str());

        remove(boxToRemove);
        playerBoxes.erase(it);
        delete boxToRemove;

        updateLayout();
        redraw();
    }
    else {
        printf("Player not found for removal: %s\n", username.c_str());
    }
}

// Updates the layout of player boxes
void PlayerDisplay::updateLayout() {
    int yOffset = 10;
    for (auto box : playerBoxes) {
        box->resize(x() + 10, y() + yOffset, w() - 20, 20);
        yOffset += 30; // Adjust spacing between boxes
    }
    redraw();
}

