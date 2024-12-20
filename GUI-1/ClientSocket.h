#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <string>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "PlayerDisplay.hpp" // Include PlayerDisplay header
#include "Settings.h"        // Include Settings header
#include <tuple>

class MainWindow;

class ClientSocket {
public:
    // Constructors
    explicit ClientSocket(SOCKET socket, PlayerDisplay* playerDisplay, const std::string& settings);
    ClientSocket(const std::string& ipAddress, int port, const std::string& username,
        PlayerDisplay* playerDisplay, const std::string& settings, MainWindow* mainWindow);

    // Destructor
    ~ClientSocket();

    // Methods
    void setUsername(const std::string& username);
    const std::string& getUsername() const;
    void send(const std::string& message);
    bool receive(std::string& message);
    void changeUsername(const std::string& newUsername);
    void addingPlayer(const std::string& username);
    void removingPlayer(const std::string& username);
    void updateLocalSettings(const std::string& settingsData);
    void applyUserSettings(); // New
    void updateResolution(int width, int height); // New
    void toggleDarkMode(); // New
    bool closed();

    Settings m_settings;          // Reference to Settings
    PlayerDisplay* playerDisplay;  // Pointer to PlayerDisplay

private:
    friend struct ServerSocket;
    SOCKET m_socket;
    bool m_closed;
    std::string m_username;
    MainWindow* mainWindow;
    
};

#endif // CLIENTSOCKET_H
