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

    MainWindow* mainWindow = new MainWindow(800, 600);
    mainWindow->show();

    int result = Fl::run();
    WSACleanup();

    return result;
}
