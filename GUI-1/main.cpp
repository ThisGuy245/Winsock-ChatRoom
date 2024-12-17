#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <winsock2.h>
#include "MainWindow.h"
#include "pugixml.hpp"

// Entry point for the application
int main(int argc, char** argv) {
    // Initialize Winsock
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fl_alert("Failed to initialize Winsock.");
        return 1;
    }

    // Create the main window
    MainWindow* mainWindow = new MainWindow(1280, 720);

    // Show the main window
    mainWindow->show(argc, argv);

    // Run the FLTK event loop
    int result = Fl::run();

    // Clean up Winsock before exiting
    WSACleanup();

    return result;
}
