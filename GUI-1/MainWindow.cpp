#include "MainWindow.h"
#include "HomePage.hpp"
#include "LobbyPage.hpp"
#include <FL/fl_ask.H> // For debugging alerts
#include <memory>
#include "Settings.h"  // Include Settings for loading resolution

/**
 * @class MainWindow
 * @brief The main application window managing different pages (HomePage and LobbyPage) and timer functionality.
 */

MainWindow::MainWindow(int width, int height)
    : Fl_Window(width, height), homePage(nullptr), lobbyPage(nullptr), timer(0.1) {


    // Load resolution from settings XML
    Settings settings("config.xml"); // Path to your settings file
    std::string username = settings.getUsername();  // Get the username
    pugi::xml_node userNode = settings.findClient(username);  // Get the XML node for that username
    auto [savedWidth, savedHeight] = settings.getRes(userNode);  // Get the resolution

    // Resize window with the saved resolution
    resize(0, 0, savedWidth, savedHeight);

    // Make the window resizable
    resizable(this);

    // Initialize homePage
    homePage = new HomePage(0, 0, savedWidth, savedHeight, this);

    if (!homePage) {
        fl_alert("Failed to create HomePage!");
        return;
    }
    add(homePage);
    homePage->show();


    // Initialize lobbyPage
    lobbyPage = new LobbyPage(0, 0, savedWidth, savedHeight);

    if (!lobbyPage) {
        fl_alert("Failed to create LobbyPage!");
        return;
    }
    add(lobbyPage);
    lobbyPage->hide();


    // Timer for periodic updates

    timer.setCallback([](void* userdata) {
        auto* window = static_cast<MainWindow*>(userdata);
        if (window) {
            window->onTick(userdata);
        }
        });
    timer.setUserData(this);
    timer.start();

    // Set the minimum window size
    size_range(800, 600, 10000, 10000);
}

/**
 * @brief Destructor to clean up dynamically allocated pages.
 */
MainWindow::~MainWindow() {
    delete homePage;
    delete lobbyPage;
    close();
}

/**
 * @brief Resizes the window and adjusts child widget dimensions.
 * @param X The new X position of the window.
 * @param Y The new Y position of the window.
 * @param W The new width of the window.
 * @param H The new height of the window.
 */
void MainWindow::resize(int X, int Y, int W, int H) {
    Fl_Window::resize(X, Y, W, H);

    // Save the resolution to settings
    Settings settings("settings.xml"); // You can pass the path to your settings file
    settings.setRes(settings.getUsername(), W, H); // Set the resolution for the current user

    if (homePage) {
        homePage->resize(0, 0, W, H);
    }
    if (lobbyPage) {
        lobbyPage->resize(0, 0, W, H);
        lobbyPage->resizeWidgets(0, 0, W, H); // Resize the widgets inside LobbyPage
    }
}


/**
 * @brief Timer callback to update the LobbyPage.
 * @param userdata Pointer to the MainWindow instance.
 */
void MainWindow::onTick(void* userdata) {
    if (lobbyPage && lobbyPage->visible()) {
        lobbyPage->Update();
    }

    // Repeat the timer callback every 1 second
    Fl::repeat_timeout(1.0, [](void* userdata) {
        auto* window = static_cast<MainWindow*>(userdata);
        if (window) {
            window->onTick(userdata);
        }
        }, userdata);
}

/**
 * @brief Sets the resolution (size) of the window.
 * @param width The new width of the window.
 * @param height The new height of the window.
 */
void MainWindow::setResolution(int width, int height) {
    this->size(width, height); // Resizes the window
    this->redraw();
}

/**
 * @brief Returns a pointer to the LobbyPage instance.
 * @return Pointer to the LobbyPage instance.

 */
LobbyPage* MainWindow::getLobbyPage() const {
    return lobbyPage;
}

/**
 * @brief Switches to the HomePage view.
 * @param widget FLTK widget pointer (unused).
 * @param userdata Pointer to the MainWindow instance.
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
 * @brief Switches to the LobbyPage view.
 * @param widget FLTK widget pointer (unused).
 * @param userdata Pointer to the MainWindow instance.
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
 * @param callback The function to be called on close.
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
