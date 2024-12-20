#include "MyWidget.h"
#include <iostream>

/**
 * @brief Constructor for MyWidget, initializes widget properties and prepares the image for drawing
 *
 * @param _x The x position of the widget
 * @param _y The y position of the widget
 * @param _w The width of the widget
 * @param _h The height of the widget
 */
MyWidget::MyWidget(int _x, int _y, int _w, int _h)
    : Fl_Widget(_x, _y, _w, _h, "gay")
    , m_drawbox("DrawBox.png")   ///< Image object for drawing
    , m_x(-1)                    ///< Initial X position for drawing
    , m_y(-1)                    ///< Initial Y position for drawing
    , m_w(0)                     ///< Initial width for the drawn image
    , m_h(0)                     ///< Initial height for the drawn image
{
}

/**
 * @brief Draws the widget and the image on the widget if coordinates are set
 */
void MyWidget::draw()
{
    // Draw the background box
    fl_draw_box(FL_DOWN_BOX, x(), y(), w(), h(), fl_rgb_color(255, 255, 255));

    // If coordinates are set, draw the image at the specified location
    if (m_x != -1 && m_y != -1) {
        m_drawbox.draw(m_x, m_y, m_w, m_h);
    }
}

/**
 * @brief Handles mouse events to allow for interactive drawing of the image
 *
 * @param _event The event code that specifies the type of interaction (e.g., FL_PUSH, FL_DRAG, FL_RELEASE)
 * @return int Returns 1 if the event is handled, 0 otherwise
 */
int MyWidget::handle(int _event)
{
    switch (_event) {
    case FL_PUSH:   ///< Mouse button pressed
        m_x = Fl::event_x();  ///< Record the starting X position
        m_y = Fl::event_y();  ///< Record the starting Y position
        m_w = 0;               ///< Set width to 0 initially
        m_h = 0;               ///< Set height to 0 initially
        redraw();              ///< Redraw widget with new state
        return 1;

    case FL_DRAG:   ///< Mouse dragged
        m_w = Fl::event_x() - m_x;  ///< Calculate width based on drag distance
        m_h = Fl::event_y() - m_y;  ///< Calculate height based on drag distance
        redraw();                    ///< Redraw widget to reflect new size
        return 1;

    case FL_RELEASE:   ///< Mouse button released
        std::cout << "Image drawn at (" << m_x << ", " << m_y << ") with size (" << m_w << ", " << m_h << ")\n";
        m_x = -1;       ///< Reset coordinates after drawing is complete
        m_y = -1;       ///< Reset coordinates after drawing is complete
        redraw();       ///< Redraw widget to reset state
        return 1;

    default:
        return Fl_Widget::handle(_event);  ///< Pass event to base class for default handling
    }
}
