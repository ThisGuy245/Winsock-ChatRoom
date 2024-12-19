#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include "pugiconfig.hpp"
#include "pugixml.hpp"
#include <tuple>

class Settings
{
public:
    explicit Settings(const std::string& path);
    ~Settings();

    pugi::xml_node findOrCreateClient(const std::string& username);

    pugi::xml_node findClient(const std::string& username);

    // Getters and setters for client-specific settings
    std::string getMode(pugi::xml_node user);
    void setMode(const std::string& username, const std::string& mode);

    std::tuple<int, int> getRes(pugi::xml_node user);
    void setRes(const std::string& username, int width, int height);



    // Save changes to the XML
    void save();

private:
    std::string m_path;     ///< Path to the XML file
    pugi::xml_document m_doc; ///< PugiXML document instance

    
};

#endif // SETTINGS_H
