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

/**
 * @class MainWindow
 * @brief Main window managing the HomePage, LobbyPage, and periodic updates.
 */
class MainWindow : public Fl_Window {
public:
    MainWindow(int width, int height); ///< Constructor
    ~MainWindow(); ///< Destructor

    void setResolution(int width, int height); ///< Resize window
    LobbyPage* getLobbyPage() const; ///< Get LobbyPage
    static void switch_to_home(Fl_Widget* widget, void* userdata); ///< Switch to HomePage
    static void switch_to_lobby(Fl_Widget* widget, void* userdata); ///< Switch to LobbyPage
    void on_close(const std::function<void()>& callback); ///< Set close callback
    void close(); ///< Close the window
    void resize(int X, int Y, int W, int H); ///< Resize the window and widgets
    void onTick(void* userdata); ///< Timer callback to update the LobbyPage

    HomePage* homePage; ///< Pointer to HomePage
    LobbyPage* lobbyPage; ///< Pointer to LobbyPage
    std::vector<std::function<void()>> close_callbacks; ///< Cleanup callbacks

private:
    Timer timer; ///< Timer instance for updates
};

#endif // MAINWINDOW_H
