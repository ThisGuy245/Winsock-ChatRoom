#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <FL/Fl_Window.H>
#include <functional>
#include <vector>
#include "Timer.h" // Include the Timer header

#include "HomePage.hpp"
#include "LobbyPage.hpp"

// Forward declarations
class HomePage;
class LobbyPage;

class MainWindow : public Fl_Window {
public:
    MainWindow(int width, int height);
    ~MainWindow();

    LobbyPage* getLobbyPage() const;
    static void switch_to_home(Fl_Widget* widget, void* userdata);
    static void switch_to_lobby(Fl_Widget* widget, void* userdata);
    void on_close(const std::function<void()>& callback);
    void close();
    void resize(int X, int Y, int W, int H);
    void onTick(void* userdata);

    HomePage* homePage;
    LobbyPage* lobbyPage;
    std::vector<std::function<void()>> close_callbacks;

private:


    Timer timer;
};

#endif // MAINWINDOW_H