#pragma once
#include <FL/Fl_Widget.H>
#include <FL/Fl_draw.H>
#include <FL/Fl_PNG_Image.H>

struct MyWidget : Fl_Widget
{
	MyWidget(int _x, int _y, int _w, int _h);

private:
	void draw();
	int handle(int _event);

	Fl_PNG_Image m_drawbox;
	int m_x;
	int m_y;
	int m_w;
	int m_h;

};
