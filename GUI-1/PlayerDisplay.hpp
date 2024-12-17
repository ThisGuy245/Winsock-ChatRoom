#ifndef PLAYERDISPLAY_HPP
#define PLAYERDISPLAY_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>
#include <vector>
#include <string>

class PlayerDisplay : public Fl_Group {
private:
    Fl_Scroll* scrollArea;                // Scrollable area for player names
    std::vector<Fl_Box*> playerBoxes;     // List of player name boxes

public:
    PlayerDisplay(int X, int Y, int W, int H);
    ~PlayerDisplay();

    void clear();                          // Clears all players from display
    void addPlayer(const std::string& username);  // Adds a player to the display
    void removePlayer(const std::string& playerName);  // Removes a player by name
    void updateLayout();                   // Reorganizes layout after removal
};

#endif // PLAYERDISPLAY_HPP
