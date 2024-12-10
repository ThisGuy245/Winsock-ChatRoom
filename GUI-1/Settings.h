#pragma once
#include <string>
#include "pugiconfig.hpp"
#include "pugixml.hpp"

struct Settings
{
public:
	Settings(const std::string& _path);
	~Settings();

	int getWidth();
	void setWidth(int _width);
	int getHeight();
	void setHeight(int _height);

	void testing();

private:
	pugi::xml_document m_doc;
};

