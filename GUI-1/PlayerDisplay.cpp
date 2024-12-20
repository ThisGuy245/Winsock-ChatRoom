#include "PlayerDisplay.hpp"
#include <FL/fl_draw.H>
#include <cstdio> // For printf debugging

/**
 * @brief Constructor for the PlayerDisplay class.
 *
 * Sets up the player list display and initializes the text buffer to hold player names.
 *
 * @param X The X position of the display.
 * @param Y The Y position of the display.
 * @param W The width of the display.
 * @param H The height of the display.
 */
PlayerDisplay::PlayerDisplay(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H), disp(nullptr), tbuff(nullptr) {
    begin();

    // Create the text display widget for the player list
    disp = new Fl_Text_Display(X + W / 2, Y, W / 2, H - 40); // Adjusted position and width
    tbuff = new Fl_Text_Buffer(); // Create the text buffer
    disp->buffer(tbuff); // Assign the text buffer to the display widget
    tbuff->text("Players:\n"); // Initialize the player list with the header text

    end(); // Finalize the group
}

/**
 * @brief Destructor for the PlayerDisplay class.
 *
 * Cleans up allocated widgets and memory.
 */
PlayerDisplay::~PlayerDisplay() {
    delete disp;  // Clean up the text display widget
    delete tbuff; // Clean up the text buffer
}

/**
 * @brief Adds a new player to the display.
 *
 * This method appends the player's username to the displayed player list.
 *
 * @param username The username to add to the display.
 */
void PlayerDisplay::addPlayer(const std::string& username) {
    // Debugging output to track the addition of a player
    printf("Adding player: %s\n", username.c_str());

    // Append the username to the text buffer, followed by a newline
    tbuff->append((username + "\n").c_str());
    redraw(); // Redraw the widget to reflect the new player
}

/**
 * @brief Removes a player from the display.
 *
 * This method searches for the player's username in the text buffer and removes it.
 *
 * @param username The username to remove from the display.
 */
void PlayerDisplay::removePlayer(const std::string& username) {
    // Debugging output to track the removal of a player
    printf("Removing player: %s\n", username.c_str());

    // Retrieve the current text from the buffer
    const char* text = tbuff->text();
    std::string bufferText(text); // Convert to a string for easier manipulation
    size_t pos = bufferText.find(username); // Find the position of the username

    // If the username is found, remove it along with the newline character
    if (pos != std::string::npos) {
        size_t endPos = bufferText.find("\n", pos); // Find the end of the username line
        if (endPos != std::string::npos) {
            bufferText.erase(pos, endPos - pos + 1); // Remove the username and the newline
            tbuff->text(bufferText.c_str()); // Update the text buffer with the modified text
        }
    }

    redraw(); // Redraw the widget after modifying the player list
}

/**
 * @brief Updates the layout of the player display.
 *
 * This function is a placeholder for future layout updates, such as resizing player boxes.
 * Currently, it only forces a redraw.
 */
void PlayerDisplay::updateLayout() {
    // Placeholder for future layout updates
    redraw(); // Redraw the widget
}
