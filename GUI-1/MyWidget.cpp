#include "MyWidget.h"
#include <iostream>

MyWidget::MyWidget(int _x, int _y, int _w, int _h)
    : Fl_Widget(_x, _y, _w, _h, "gay")
    , m_drawbox("DrawBox.png")
    , m_x(-1)
    , m_y(-1)
    , m_w(0)
    , m_h(0)
{
}

void MyWidget::draw()
{
    fl_draw_box(FL_DOWN_BOX, x(), y(), w(), h(), fl_rgb_color(255, 255, 255));
    if (m_x != -1 && m_y != -1) {
        m_drawbox.draw(m_x, m_y, m_w, m_h);
    }
}

int MyWidget::handle(int _event)
{
    switch (_event) {
    case FL_PUSH:
        m_x = Fl::event_x();
        m_y = Fl::event_y();
        m_w = 0;
        m_h = 0;
        redraw();
        return 1;

    case FL_DRAG:
        m_w = Fl::event_x() - m_x;
        m_h = Fl::event_y() - m_y;
        redraw();
        return 1;

    case FL_RELEASE:
        std::cout << "Image drawn at (" << m_x << ", " << m_y << ") with size (" << m_w << ", " << m_h << ")\n";
        m_x = -1;
        m_y = -1;
        redraw();
        return 1;

    default:
        return Fl_Widget::handle(_event);
    }
}
