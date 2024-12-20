#include <FL/fl_ask.H>
#include "ClientSocket.h"
#include "ServerSocket.h"
#include <FL/fl_draw.H>
#include <iostream>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "MainWindow.h"

/**
 * @brief Constructor for ClientSocket with an existing socket.
 */
ClientSocket::ClientSocket(SOCKET socket, PlayerDisplay* playerDisplay, const std::string& settings)
    : m_socket(socket), m_closed(false), playerDisplay(playerDisplay), m_settings(settings), mainWindow(nullptr) {
    if (socket == INVALID_SOCKET) {
        throw std::runtime_error("Invalid socket");
    }
}

/**
 * @brief Constructor for ClientSocket when connecting to a server.
 */
ClientSocket::ClientSocket(const std::string& ipAddress, int port, const std::string& username,
    PlayerDisplay* playerDisplay, const std::string& settings, MainWindow* mainWindow)
    : m_socket(INVALID_SOCKET), m_closed(false), m_username(username),
    playerDisplay(playerDisplay), m_settings(settings), mainWindow(mainWindow) {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddress.sin_addr) <= 0) {
        closesocket(m_socket);
        WSACleanup();
        throw std::runtime_error("Invalid address / Address not supported");
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        closesocket(m_socket);
        WSACleanup();
        throw std::runtime_error("Failed to connect to server");
    }

    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    send(m_username); // Send username to the server
    applyUserSettings(); // Apply user settings on connection
}



/**
 * @brief Destructor for ClientSocket.
 */
ClientSocket::~ClientSocket() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
}

/**
 * @brief Apply user-specific settings from the Settings class.
 */
void ClientSocket::applyUserSettings() {
    pugi::xml_node user = m_settings.findOrCreateClient(m_username);

    // Apply resolution from settings
    std::tuple<int, int> resolution = m_settings.getRes(user);
    int width = std::get<0>(resolution);
    int height = std::get<1>(resolution);

    // Apply the resolution to the main window
    if (mainWindow) {
        mainWindow->setResolution(width, height);
    }

    // Apply dark mode
    std::string mode = m_settings.getMode(user);
    if (mode == "true") {
        // Enable dark mode
        Fl::background(45, 45, 45);  // Dark background for main window
        Fl::foreground(255, 255, 255);  // White text
    }
    else {
        // Enable light mode
        Fl::background(240, 240, 240);  // Light background
        Fl::foreground(0, 0, 0);  // Black text
    }
    if (mainWindow) {
        mainWindow->redraw();
    }
}


/**
 * @brief Sets the username for this client and updates the global player display.
 * @param username The new username to set for this client.
 */
void ClientSocket::setUsername(const std::string& username) {
    m_username = username;
}


/**
 * @brief Gets the current username of this client.
 * @return The current username of the client.
 */
const std::string& ClientSocket::getUsername() const {
    return m_username;
}

/**
 * @brief Sends a message to the server.
 * @param message The message to send.
 * @throws std::runtime_error if sending the message fails.
 */
void ClientSocket::send(const std::string& message) {
    int bytes = ::send(m_socket, message.c_str(), message.length(), 0);
    if (bytes <= 0) {
        throw std::runtime_error("Failed to send data");
    }
}

/**
 * @brief Receives a message from the server.
 * @param message The message received from the server.
 * @return True if the message was successfully received, otherwise false.
 */
bool ClientSocket::receive(std::string& message) {
    char buffer[512] = { 0 };
    int bytes = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            m_closed = true;  // Client is disconnected
            return false;
        }
        return false;
    }
    else if (bytes == 0) {  // Connection closed by the peer
        m_closed = true;
        return false;
    }

    message.assign(buffer, bytes);
    return true;
}

/**
 * @brief Sends a username change request to the server and updates the local display.
 * @param newUsername The new username to set.
 */
void ClientSocket::changeUsername(const std::string& newUsername) {
    std::string command = "/change_username " + newUsername;
    send(command);  // Send the command to the server

    // Update the local username immediately
    setUsername(newUsername);

    // Optionally, you can listen for a confirmation from the server to update the UI
    std::string response;
    if (receive(response)) {
        if (response == "USERNAME_CHANGED") {
            fl_alert("Your username has been successfully changed to '%s'.", newUsername.c_str());
        }
        else if (response == "USERNAME_TAKEN") {
            // Handle failure case (username already taken, invalid, etc.)
            fl_alert("Failed to change username. The username '%s' is already taken. Please try again.", newUsername.c_str());
        }
    }
}

/**
 * @brief Adds a player to the player list and updates the PlayerDisplay.
 * @param username The username of the player to add.
 */
void ClientSocket::addingPlayer(const std::string& username) {
    if (playerDisplay) {
        playerDisplay->addPlayer(username); // Add player to the display
    }
}

/**
 * @brief Removes a player from the player list and updates the PlayerDisplay.
 * @param username The username of the player to remove.
 */
void ClientSocket::removingPlayer(const std::string& username) {
    if (playerDisplay) {
        playerDisplay->removePlayer(username); // Remove player from the display
    }
}


/**
 * @brief Update the user's resolution settings.
 */
void ClientSocket::updateResolution(int width, int height) {
    m_settings.setRes(m_username, width, height);
    fl_message("Resolution updated to: %dx%d", width, height);
}

/**
 * @brief Toggle the user's dark mode setting.
 */
void ClientSocket::toggleDarkMode() {
    pugi::xml_node user = m_settings.findOrCreateClient(m_username);
    std::string currentMode = m_settings.getMode(user);
    std::string newMode = (currentMode == "true") ? "false" : "true";
    m_settings.setMode(m_username, newMode);
    fl_message("Dark mode toggled to: %s", newMode.c_str());
}

/**
 * @brief Update local settings based on server data.
 */
void ClientSocket::updateLocalSettings(const std::string& settingsData) {
    pugi::xml_document doc;
    if (!doc.load_string(settingsData.c_str())) {
        std::cerr << "Failed to parse settings data from server" << std::endl;
        return;
    }

    if (!doc.save_file("client_settings.xml")) {
        std::cerr << "Failed to save settings to local file" << std::endl;
    }
    else {
        std::cout << "Settings updated and saved locally" << std::endl;
    }
}


/**
 * @brief Checks if the client socket is closed.
 * @return True if the socket is closed, otherwise false.
 */
bool ClientSocket::closed() {
    return m_closed;
}

//to accept a message from client socket
bool ClientSocket::receive(std::string& sender, std::string& message) {
	char buffer[256] = { 0 }; // Increased buffer size for longer messages
	int bytes = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
	if (bytes == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			m_closed = true;
			return false;
		}
		return false;
	}
	else if (bytes == 0) {
		m_closed = true;
		return false;
	}

	std::string receivedData = buffer;

	// Parse the sender and message
	size_t delimiterPos = receivedData.find(": ");
	if (delimiterPos != std::string::npos) {
		sender = receivedData.substr(0, delimiterPos);  // Extract username
		message = receivedData.substr(delimiterPos + 2); // Extract message
	}
	else {
		sender = "Unknown"; // Fallback if no delimiter found
		message = receivedData;
	}
	return true;
}


//to send a message to client socket
void ClientSocket::send(const std::string& username, const std::string& message) {
	std::string taggedMessage = username + ": " + message;
	int bytes = ::send(m_socket, taggedMessage.c_str(), taggedMessage.length(), 0);
	if (bytes <= 0) {
		throw std::runtime_error("Failed to send data");
	}
}


bool ClientSocket::closed()
{
	return m_closed;
}

