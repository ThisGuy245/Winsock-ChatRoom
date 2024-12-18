#ifndef PLAYER_DISPLAY_HPP
#define PLAYER_DISPLAY_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <vector>
#include <string>

class PlayerDisplay : public Fl_Group {
private:
    Fl_Box* sidePanel;   
    std::vector<Fl_Box*> playerBoxes;

    void updateLayout();

public:
    PlayerDisplay(int X, int Y, int W, int H);
    ~PlayerDisplay();

    void addPlayer(const std::string& username);
    void removePlayer(const std::string& username);
};

#endif
