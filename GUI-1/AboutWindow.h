#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>  // Use Fl_Text_Display here instead of Fl_Text_Editor
#include <FL/Fl_Text_Buffer.H>   // For text buffer class
#include <string>

class AboutWindow : public Fl_Window {
public:
    AboutWindow(int width, int height, const char* title = "About");
    ~AboutWindow();

    void show_about_info();

private:
    Fl_Button* close_button;
    Fl_Text_Display* info_text;  // Change to Fl_Text_Display* here
    Fl_Text_Buffer* buffer;

    static void close_callback(Fl_Widget* widget, void* userdata);
};

#endif // ABOUTWINDOW_H
