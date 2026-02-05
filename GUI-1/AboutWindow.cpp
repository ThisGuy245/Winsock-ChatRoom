#include "AboutWindow.h"
#include <FL/fl_ask.H>           // For alerts
#include <FL/Fl_Text_Display.H>  // For text display widget
#include <FL/Fl_Button.H>        // For button widget
#include <FL/Fl_Window.H>        // For window class
#include <FL/Fl_Text_Buffer.H>   // For text buffer class
#include <FL/fl_draw.H>          // For fl_rgb_color

/**
 * @brief Constructor initializes the AboutWindow UI with information and a close button
 *
 * @param width The width of the window
 * @param height The height of the window
 * @param title The title of the window
 */
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

/**
 * @brief Destructor that cleans up dynamically allocated resources.
 */
AboutWindow::~AboutWindow() {
    // Cleanup resources
    delete close_button;
    delete info_text;
    delete buffer;
}

/**
 * @brief Populates the About window with the application's information
 */
void AboutWindow::show_about_info() {
    // About page content with detailed features
    std::string about_info =
        "Welcome to the Chat Application!\n\n"
        "This application allows you to chat with others, join rooms, "
        "send messages, and more.\n\n"

        "Features:\n"
        "1. **User Authentication**: Enter your username on the home page to get started.\n"
        "2. **Room Management**: Create or join chat rooms to interact with other users.\n"
        "3. **Real-Time Messaging**: Send and receive messages in real-time with other participants.\n"
        "4. **Private Messaging**: Send private messages to specific users in the room.\n"
        "5. **Dark Mode**: Toggle dark mode for a more comfortable night-time experience.\n"
        "6. **Chat History**: View your past conversations stored for future reference.\n"
        "7. **Customizable Settings**: Adjust user preferences like username, theme, and resolution.\n"
        "8. **User List**: View a list of connected users in the current room.\n"
        "9. **Message Notifications**: Receive notifications for new messages while active.\n\n"

        "How to Use the Features:\n"
        "1. Check Server Version via SV/.\n"
        "2. Whisper feature in the form W/[user]'.\n"
        "3. Dark mode and Change username in Settings\n\n"

        "Developed by: Thomas Isherwood\n"
        "Version: 1.0";

    buffer->text(about_info.c_str()); // Set the text in the display
}


/**
 * @brief Applies dark or light theme to the About window.
 * @param isDarkMode True for dark mode, false for light mode.
 */
void AboutWindow::applyTheme(bool isDarkMode) {
    Fl_Color bgColor = isDarkMode ? fl_rgb_color(45, 45, 45) : fl_rgb_color(255, 255, 255);
    Fl_Color textColor = isDarkMode ? FL_WHITE : FL_BLACK;
    Fl_Color windowBg = isDarkMode ? fl_rgb_color(60, 60, 60) : fl_rgb_color(240, 240, 240);
    
    // Window background
    this->color(windowBg);
    
    // Info text display
    if (info_text) {
        info_text->color(bgColor);
        info_text->textcolor(textColor);
        info_text->redraw();
    }
    
    // Close button
    if (close_button) {
        close_button->color(isDarkMode ? fl_rgb_color(70, 70, 70) : FL_LIGHT2);
        close_button->labelcolor(textColor);
        close_button->redraw();
    }
    
    redraw();
}

/**
 * @brief Callback function to close the About window when the close button is clicked
 *
 * @param widget The widget that triggered the callback
 * @param userdata The user data (AboutWindow instance)
 */
void AboutWindow::close_callback(Fl_Widget* widget, void* userdata) {
    AboutWindow* page = static_cast<AboutWindow*>(userdata);
    if (page) {
        page->hide();
    }
}
