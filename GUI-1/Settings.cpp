#include "Settings.h"
#include <iostream>
#include <tuple>
#include <FL/fl_ask.H>

// Constructor: Load the XML file
Settings::Settings(const std::string& path) : m_path(path)
{
    pugi::xml_parse_result result = m_doc.load_file(m_path.c_str());
    if (!result)
    {
        std::cerr << "Failed to load XML file: " << result.description() << std::endl;
        m_doc.append_child("Clients");
        save(); // Save initial structure
    }
}

// Destructor: Save XML on destruction
Settings::~Settings()
{
    save();
}

// Find or create a client node by username
pugi::xml_node Settings::findOrCreateClient(const std::string& username)
{
    pugi::xml_node clientsNode = m_doc.child("Clients");
    for (pugi::xml_node client : clientsNode.children("Client"))
    {
        std::string test = std::string(client.child("Username").text().as_string());
        if (test == username) {
            return client;
        }
    }
    // Create a new client if not found
    pugi::xml_node newClient = clientsNode.append_child("Client");
    newClient.append_child("Username").text() = username.c_str();

    newClient.append_child("Dark").text() = "false"; // Default to light mode
    pugi::xml_node resolution = newClient.append_child("Resolution");
    resolution.append_child("Width").text() = 800; // Default width
    resolution.append_child("Height").text() = 600; // Default height
    save();
    return newClient;
}

pugi::xml_node Settings::findClient(const std::string& username)
{
    pugi::xml_node clientsNode = m_doc.child("Clients");
    for (pugi::xml_node client : clientsNode.children("Client"))
    {
        std::string test = std::string(client.child("Username").text().as_string());
        if (test == username) {
            return client;
        }
    }
    fl_alert("Cannot find user: ", username);
    return pugi::xml_node(); // Return an empty node if not found
}

// Get mode
std::string Settings::getMode(pugi::xml_node user)
{
    return user.child("Dark").text().as_string();
}

// Set mode
void Settings::setMode(const std::string& username, const std::string& mode)
{
    findOrCreateClient(username).child("Dark").text() = mode.c_str();
    save();
}

void Settings::setUsername(const std::string& newUsername) {
    pugi::xml_node user = findOrCreateClient(newUsername);
    user.child("Username").text() = newUsername.c_str();
    save();  // Make sure this saves the updated settings to the XML file
}

std::tuple<int, int> Settings::getRes(pugi::xml_node user)
{
    pugi::xml_node res = user.child("Resolution");
    return std::make_tuple(res.child("Width").text().as_int(), res.child("Height").text().as_int());
}

// Set height
void Settings::setRes(const std::string& username, int height, int width)
{
    findOrCreateClient(username).child("Resolution").child("Width").text() = width;
    findOrCreateClient(username).child("Resolution").child("Height").text() = height;
    save();
}

// Save changes to the XML file
void Settings::save()
{
    if (!m_doc.save_file(m_path.c_str()))
    {
        std::cerr << "Failed to save XML file: " << m_path << std::endl;
    }
}

// Get the username from the settings
std::string Settings::getUsername() {
    // Retrieve the first client's username as an example (you can modify this for your needs)
    pugi::xml_node client = m_doc.child("Clients").child("Client");
    return client.child("Username").text().as_string();
}

// Load settings (can be a placeholder for now)
void Settings::loadSettings() {
    // Load settings from the XML if needed (in this case it's done in the constructor)
}

// Save settings (already handled in the save method)
void Settings::saveSettings() {
    save();
}
