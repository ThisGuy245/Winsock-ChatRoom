/*! \mainpage Chat Application Documentation

\section intro_sec Introduction

This is the documentation for the **Chat Application** project, a C++ application built using **FLTK for GUI** and **Winsock2 for networking**.

\section purpose_sec Project Purpose

The project implements:
- A **client/server architecture** for communication.
- A **GUI-based interface** for the chat system.
- **XML-based data management** for chat history and user settings.

\section structure_sec Project Structure

The project consists of:
- **MainWindow**: Handles page switching and periodic updates.
- **HomePage**: Lets users configure their username and connection settings.
- **LobbyPage**: Core chat functionality with messaging and player display.
- **ServerSocket/ClientSocket**: Networking modules for client and server communication.
- **SettingsWindow**: Manages resolution, username changes, and themes.

\section contact_sec Contact

For any inquiries, please contact: **Thomas Isherwood**

*/

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <winsock2.h>
#include "MainWindow.h"
#include "pugixml.hpp"

/*! \brief Entry point for the application
 *
 *  Initializes Winsock, creates the main window, and runs the FLTK event loop.
 *  \param argc Number of command-line arguments
 *  \param argv Array of command-line arguments
 *  \return Exit status of the application
 */
    int main(int argc, char** argv) {
    // Initialize Winsock
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fl_alert("Failed to initialize Winsock.");
        return 1;
    }

    // Create the main window
    MainWindow* mainWindow = new MainWindow(1100, 800);

    // Show the main window
    mainWindow->show(argc, argv);

    // Run the FLTK event loop
    int result = Fl::run();

    // Clean up Winsock before exiting
    WSACleanup();

    return result;
}