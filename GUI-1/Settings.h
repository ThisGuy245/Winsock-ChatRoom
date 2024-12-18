#pragma once
#include <string>
#include "pugiconfig.hpp"
#include "pugixml.hpp"

class Settings
{
public:
    Settings(const std::string& path);
    ~Settings();

    std::string getMode();
    void setMode(const std::string& mode);

    int getWidth();
    void setWidth(int width);

    int getHeight();
    void setHeight(int height);

    void save(); // Save changes to the XML file

private:
    std::string m_path;          // Path to the XML file
    pugi::xml_document m_doc;    // XML document object
};

