#ifndef PLAYER_DISPLAY_HPP
#define PLAYER_DISPLAY_HPP

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <string>

class PlayerDisplay : public Fl_Group {
public:
    PlayerDisplay(int X, int Y, int W, int H);
    ~PlayerDisplay();

    void addPlayer(const std::string& username);
    void removePlayer(const std::string& username);
    void updateLayout();

private:
    Fl_Box* sidePanel;
    Fl_Text_Display* disp;
    Fl_Text_Buffer* tbuff;
};

#endif // PLAYER_DISPLAY_HPP