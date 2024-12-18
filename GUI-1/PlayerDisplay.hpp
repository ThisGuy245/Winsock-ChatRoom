#ifndef PLAYERDISPLAY_HPP
#define PLAYERDISPLAY_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>
#include <vector>
#include <string>

class PlayerDisplay : public Fl_Group {
private:
    Fl_Scroll* scrollArea;
    Fl_Box* playersLabel;
    std::vector<Fl_Box*> playerBoxes;  // List of player display boxes
    std::vector<std::pair<std::string, bool>> playerStatus; // Player and connection status

public:
    PlayerDisplay(int X, int Y, int W, int H);
    ~PlayerDisplay();

    void addPlayer(const std::string& username);
    void removePlayer(const std::string& username);
    void updatePlayerStatus(const std::string& username, bool isConnected);
    void clear();  // Clears the display
    std::vector<std::string> getPlayers();

private:
    void updateLayout();  // Adjusts layout after player removal
};

#endif // PLAYERDISPLAY_HPP