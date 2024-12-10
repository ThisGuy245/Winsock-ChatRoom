#include "MainWindow.h"
#include "HomePage.h"
#include "LobbyPage.h"
#include <FL/fl_ask.H> // For debugging alerts

// Constructor: Initializes MainWindow with specified dimensions
MainWindow::MainWindow(int width, int height)
    : Fl_Window(width, height), homePage(nullptr), lobbyPage(nullptr) {
    // Create and initialize the Home Page
    homePage = new HomePage(0, 0, width, height, this);
    add(homePage); // Add HomePage to the MainWindow
    homePage->show(); // Start with the Home Page visible

    // Create and initialize the Lobby Page (hidden by default)
    lobbyPage = new LobbyPage(0, 0, width, height);
    add(lobbyPage); // Add LobbyPage to the MainWindow
    lobbyPage->hide(); // Hide Lobby Page initially
}

// Destructor: Cleans up dynamically allocated pages
MainWindow::~MainWindow() {
    // Ensure all allocated pages are deleted
    delete homePage;
    delete lobbyPage;

    // Execute cleanup logic
    close();
}

// Getter: Returns the LobbyPage instance
LobbyPage* MainWindow::getLobbyPage() const {
    return lobbyPage;
}

// Static method: Switch to the Home Page
void MainWindow::switch_to_home(Fl_Widget* widget, void* userdata) {
    auto* window = static_cast<MainWindow*>(userdata);
    if (!window) {
        fl_alert("Failed to retrieve MainWindow instance for switching pages.");
        return;
    }

    if (window->lobbyPage) window->lobbyPage->hide();
    if (window->homePage) window->homePage->show();
    window->redraw(); // Ensure the UI updates
}

// Static method: Switch to the Lobby Page
void MainWindow::switch_to_lobby(Fl_Widget* widget, void* userdata) {
    auto* window = static_cast<MainWindow*>(userdata);
    if (!window) {
        fl_alert("Failed to retrieve MainWindow instance for switching pages.");
        return;
    }

    if (window->homePage) window->homePage->hide();
    if (window->lobbyPage) window->lobbyPage->show();
    window->redraw(); // Ensure the UI updates
}

// Adds a cleanup callback to be executed when the window closes
void MainWindow::on_close(const std::function<void()>& callback) {
    close_callbacks.push_back(callback);
}

// Closes the application and executes registered cleanup callbacks
void MainWindow::close() {
    for (const auto& callback : close_callbacks) {
        callback(); // Execute each registered callback
    }
    hide(); // Close the window
}
