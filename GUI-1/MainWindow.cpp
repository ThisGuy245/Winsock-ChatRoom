#include "MainWindow.h"
#include "HomePage.hpp"
#include "LobbyPage.hpp"
#include <FL/fl_ask.H> // For debugging alerts
#include <memory>

MainWindow::MainWindow(int width, int height)
    : Fl_Window(width, height), homePage(nullptr), lobbyPage(nullptr), timer(0.1) {

    // Make the window resizable
    resizable(this);

    // Initialize homePage
    homePage = new HomePage(0, 0, width, height, this);
    if (!homePage) {
        fl_alert("Failed to create HomePage!");
        return;
    }
    add(homePage);
    homePage->show();

    // Initialize lobbyPage
    lobbyPage = new LobbyPage(0, 0, width, height);
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
    size_range(750, 500, 10000, 10000);
}

// Destructor: Cleans up dynamically allocated pages
MainWindow::~MainWindow() {
    // Ensure all allocated pages are deleted
    delete homePage;
    delete lobbyPage;
    close();
}

// Resize handler to adjust child widget dimensions dynamically
void MainWindow::resize(int X, int Y, int W, int H) {
    Fl_Window::resize(X, Y, W, H);

    if (homePage) {
        homePage->resize(0, 0, W, H);
    }
    if (lobbyPage) {
        lobbyPage->resize(0, 0, W, H);
        lobbyPage->resizeWidgets(0, 0, W, H); // Resize the widgets inside LobbyPage
    }
}

// Timer tick callback to update the LobbyPage
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

void MainWindow::setResolution(int width, int height) {
    this->size(width, height); // Resizes the window
    this->redraw();
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

    if (window->lobbyPage) {
        window->lobbyPage->hide();
    }
    if (window->homePage) {
        window->homePage->show();
    }
    window->redraw(); // Ensure the UI updates
}

// Static method: Switch to the Lobby Page
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
        fl_alert("lobbyPage is not initialized!");
    }
    window->redraw(); // Ensure the UI updates
}

// Adds a cleanup callback to be executed when the window closes
void MainWindow::on_close(const std::function<void()>& callback) {
    close_callbacks.push_back(callback);
}

// Closes the application and executes registered callbacks
void MainWindow::close() {
    for (const auto& callback : close_callbacks) {
        callback();
    }
    hide(); // Close the window
}
