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
 * @brief The main application window that manages switching between pages and handling application events.
 */
class MainWindow : public Fl_Window {
public:
    /**
     * @brief Constructor for MainWindow.
     * @param width Width of the window.
     * @param height Height of the window.
     */
    MainWindow(int width, int height);

    /**
     * @brief Destructor for MainWindow.
     */
    ~MainWindow();

    /**
     * @brief Retrieves the LobbyPage instance.
     * @return Pointer to the LobbyPage instance.
     */
    LobbyPage* getLobbyPage() const;

    /**
     * @brief Static method to switch to the Home Page.
     * @param widget FLTK widget pointer (unused).
     * @param userdata Pointer to the MainWindow instance.
     */
    static void switch_to_home(Fl_Widget* widget, void* userdata);

    /**
     * @brief Static method to switch to the Lobby Page.
     * @param widget FLTK widget pointer (unused).
     * @param userdata Pointer to the MainWindow instance.
     */
    static void switch_to_lobby(Fl_Widget* widget, void* userdata);

    /**
     * @brief Adds a cleanup callback to be executed when the window closes.
     * @param callback A function to be called on close.
     */
    void on_close(const std::function<void()>& callback);

    /**
     * @brief Closes the application and executes registered cleanup callbacks.
     */
    void close();

    /**
     * @brief Timer tick callback to handle periodic updates.
     * @param userdata Pointer to the MainWindow instance.
     */
    void onTick(void* userdata);

    /**
     * @brief Pointer to the HomePage instance.
     */
    HomePage* homePage;

    /**
     * @brief Pointer to the LobbyPage instance.
     */
    LobbyPage* lobbyPage;

    /**
     * @brief List of callbacks to execute upon window close.
     */
    std::vector<std::function<void()>> close_callbacks;

private:
    /**
     * @brief Timer instance for managing periodic updates.
     */
    Timer timer;
};

#endif // MAINWINDOW_H
