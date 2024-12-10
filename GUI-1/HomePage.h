#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <FL/Fl_Group.H>

// Forward declaration of MainWindow to avoid circular dependency
class MainWindow;

class HomePage : public Fl_Group {
public:
    // Constructor accepting MainWindow pointer
    HomePage(int x, int y, int w, int h, MainWindow* mainWindow);
    ~HomePage();

private:
    MainWindow* m_mainWindow; // Pointer to the parent MainWindow

    // Static callback functions
    static void host_button_callback(Fl_Widget* widget, void* userdata);
    static void join_button_callback(Fl_Widget* widget, void* userdata);
};

#endif
