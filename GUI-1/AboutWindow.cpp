#include "AboutWindow.h"
#include <FL/fl_ask.H>           // For alerts
#include <FL/Fl_Text_Display.H>  // For text display widget
#include <FL/Fl_Button.H>        // For button widget
#include <FL/Fl_Window.H>        // For window class
#include <FL/Fl_Text_Buffer.H>   // For text buffer class

AboutWindow::AboutWindow(int width, int height, const char* title)
    : Fl_Window(width, height, title), close_button(nullptr), info_text(nullptr), buffer(nullptr) {

    this->begin();

    // Text display to show the application info (non-editable by default)
    info_text = new Fl_Text_Display(10, 10, width - 20, height - 50);
    buffer = new Fl_Text_Buffer();
    info_text->buffer(buffer);
    info_text->wrap_mode(Fl_Text_Display::WRAP_AT_PIXEL, 0);

    show_about_info();

    // Close button
    close_button = new Fl_Button(width - 110, height - 40, 100, 30, "Close");
    close_button->callback(close_callback, this);  // Set callback to close the window

    this->end();
    this->show();
}

AboutWindow::~AboutWindow() {
    // Cleanup resources
    delete close_button;
    delete info_text;
    delete buffer;
}

void AboutWindow::show_about_info() {
    // About page content (example text)
    std::string about_info = "Welcome to the Chat Application!\n\n"
        "This application allows you to chat with others, join rooms, "
        "send messages, and more.\n\n"
        "To use the application:\n"
        "1. Enter your username on the home page.\n"
        "2. Choose a room and click 'Join'.\n"
        "3. Start chatting with others!\n\n"
        "Developed by: Thomas Isherwood";

    buffer->text(about_info.c_str()); // Set the text in the display
}

void AboutWindow::close_callback(Fl_Widget* widget, void* userdata) {
    AboutWindow* page = static_cast<AboutWindow*>(userdata);
    if (page) {
        page->hide();
    }
}
