#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <FL/Fl_Window.H>
#include <functional>
#include <vector>

// Forward declarations
class HomePage;
class LobbyPage;

class MainWindow : public Fl_Window {
public:
    // Constructor: Initializes MainWindow with given dimensions
    MainWindow(int width, int height);

    // Destructor: Cleans up allocated resources
    ~MainWindow();

    // Getter: Retrieves the LobbyPage instance
    LobbyPage* getLobbyPage() const;

    // Static method: Switches the view to the Home Page
    static void switch_to_home(Fl_Widget* widget, void* userdata);

    // Static method: Switches the view to the Lobby Page
    static void switch_to_lobby(Fl_Widget* widget, void* userdata);

    // Registers a cleanup callback to execute when the window closes
    void on_close(const std::function<void()>& callback);

    // Closes the application and executes registered callbacks
    void close();

private:
    HomePage* homePage;   // Pointer to the Home Page instance
    LobbyPage* lobbyPage; // Pointer to the Lobby Page instance

    std::vector<std::function<void()>> close_callbacks; // List of cleanup callbacks
};

#endif // MAINWINDOW_H
