#include "MainWindow.h"
#include "HomePage.hpp"
#include "LobbyPage.hpp"
#include <FL/fl_ask.H> // For debugging alerts
#include <memory>

/**
 * @class MainWindow
 * @brief The main application window managing different pages (HomePage and LobbyPage) and timer functionality.
 */

MainWindow::MainWindow(int width, int height)
    : Fl_Window(width, height), homePage(nullptr), lobbyPage(nullptr), timer(0.1) {

    // Initialize HomePage
    homePage = new HomePage(0, 0, width, height, this);
    if (!homePage) {
        fl_alert("Failed to create HomePage!");
        return;
    }
    add(homePage);
    homePage->show();

    // Initialize LobbyPage
    lobbyPage = new LobbyPage(0, 0, width, height);
    if (!lobbyPage) {
        fl_alert("Failed to create LobbyPage!");
        return;
    }
    add(lobbyPage);
    lobbyPage->hide();

    // Configure timer
    timer.setCallback([](void* userdata) {
        auto* window = static_cast<MainWindow*>(userdata);
        if (window) {
            window->onTick(userdata);
        }
        });

    timer.setUserData(this);
    timer.start();
}

/**
 * @brief Destructor cleans up dynamically allocated pages.
 */
MainWindow::~MainWindow() {
    delete homePage;
    delete lobbyPage;
    close();
}

/**
 * @brief Timer tick callback to update the LobbyPage.
 * @param userdata Pointer to MainWindow instance.
 */
void MainWindow::onTick(void* userdata) {
    if (lobbyPage && lobbyPage->visible()) {
        lobbyPage->Update();
    }

    Fl::repeat_timeout(1.0, [](void* userdata) {
        auto* window = static_cast<MainWindow*>(userdata);
        if (window) {
            window->onTick(userdata);
        }
        }, userdata);
}

/**
 * @brief Getter for the LobbyPage instance.
 * @return Pointer to LobbyPage.
 */
LobbyPage* MainWindow::getLobbyPage() const {
    return lobbyPage;
}

/**
 * @brief Static method to switch to the Home Page.
 * @param widget FLTK widget pointer (unused).
 * @param userdata Pointer to MainWindow instance.
 */
void MainWindow::switch_to_home(Fl_Widget* widget, void* userdata) {
    auto* window = static_cast<MainWindow*>(userdata);
    if (!window) {
        fl_alert("Failed to retrieve MainWindow instance for switching pages.");
        return;
    }

    if (window->lobbyPage) {
        window->lobbyPage->hide();
    }
    if (window->homePage) {
        window->homePage->show();
    }
    window->redraw(); // Ensure the UI updates
}

/**
 * @brief Static method to switch to the Lobby Page.
 * @param widget FLTK widget pointer (unused).
 * @param userdata Pointer to MainWindow instance.
 */
void MainWindow::switch_to_lobby(Fl_Widget* widget, void* userdata) {
    auto* window = static_cast<MainWindow*>(userdata);
    if (!window) {
        fl_alert("Failed to retrieve MainWindow instance for switching pages.");
        return;
    }

    if (window->homePage) {
        window->homePage->hide();
    }
    if (window->lobbyPage) {
        window->lobbyPage->show();
    }
    else {
        fl_alert("LobbyPage is not initialized!");
    }
    window->redraw(); // Ensure the UI updates
}

/**
 * @brief Adds a cleanup callback to be executed when the window closes.
 * @param callback Function to be called on close.
 */
void MainWindow::on_close(const std::function<void()>& callback) {
    close_callbacks.push_back(callback);
}

/**
 * @brief Closes the application and executes registered cleanup callbacks.
 */
void MainWindow::close() {
    for (const auto& callback : close_callbacks) {
        callback();
    }
    hide(); // Close the window
}
