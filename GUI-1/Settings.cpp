#include "Settings.h"
#include <iostream>

// Constructor: Load the XML file
Settings::Settings(const std::string& path) : m_path(path)
{
    pugi::xml_parse_result result = m_doc.load_file(m_path.c_str());
    if (!result)
    {
        std::cerr << "Failed to load XML file: " << result.description() << std::endl;

        // Create a default XML structure if the file doesn't exist
        pugi::xml_node root = m_doc.append_child("settings");
        pugi::xml_node display = root.append_child("display");
        display.append_child("mode").text() = "light"; // Default to light mode
        pugi::xml_node resolution = display.append_child("resolution");
        resolution.append_child("width").text() = 1920; // Default width
        resolution.append_child("height").text() = 1080; // Default height
        m_doc.save_file(m_path.c_str());
    }
}

// Destructor: Automatically save changes to the XML file
Settings::~Settings()
{
    save();
}

// Get mode (dark/light)
std::string Settings::getMode()
{
    return m_doc.child("settings").child("display").child("mode").text().as_string();
}

// Set mode (dark/light)
void Settings::setMode(const std::string& mode)
{
    m_doc.child("settings").child("display").child("mode").text() = mode.c_str();
}

// Get width
int Settings::getWidth()
{
    return m_doc.child("settings").child("display").child("resolution").child("width").text().as_int();
}

// Set width
void Settings::setWidth(int width)
{
    m_doc.child("settings").child("display").child("resolution").child("width").text() = width;
}

// Get height
int Settings::getHeight()
{
    return m_doc.child("settings").child("display").child("resolution").child("height").text().as_int();
}

// Set height
void Settings::setHeight(int height)
{
    m_doc.child("settings").child("display").child("resolution").child("height").text() = height;
}

// Save changes to the XML file
void Settings::save()
{
    if (!m_doc.save_file(m_path.c_str()))
    {
        std::cerr << "Failed to save XML file: " << m_path << std::endl;
    }
}
