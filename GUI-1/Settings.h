#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <tuple>
#include "pugixml.hpp"
#include "pugiconfig.hpp"

class Settings {
public:
    // Constructor: Load the XML file
    Settings(const std::string& path);

    // Destructor: Save XML on destruction
    ~Settings();

    // Find or create a client node by username
    pugi::xml_node findOrCreateClient(const std::string& username);

    // Find a client node by username
    pugi::xml_node findClient(const std::string& username);

    // Get dark mode setting for a specific user
    std::string getMode(pugi::xml_node user);

    // Set dark mode setting for a specific user
    void setMode(const std::string& username, const std::string& mode);

    // Set username in the XML file
    void setUsername(const std::string& newUsername);

    // Get resolution (width, height) for a specific user
    std::tuple<int, int> getRes(pugi::xml_node user);

    // Set resolution (width, height) for a specific user
    void setRes(const std::string& username, int height, int width);

    // Save changes to the XML file
    void save();

    // Additional methods for `SettingsWindow` class
    std::string getUsername();    // Get username
    void loadSettings();          // Load settings from the file
    void saveSettings();          // Save settings to the file

private:
    pugi::xml_document m_doc;
    std::string m_path;
};

#endif // SETTINGS_H
