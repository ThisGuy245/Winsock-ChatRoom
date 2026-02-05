#ifndef LOGIN_PAGE_H
#define LOGIN_PAGE_H

/**
 * @file LoginPage.h
 * @brief Login and Registration page for user authentication
 * 
 * Provides a clean interface for users to:
 * - Login with existing credentials
 * - Register a new account
 * - View validation feedback
 */

#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Choice.H>
#include <functional>
#include <string>
#include "UserDatabase.h"
#include "Models.h"

// Forward declaration
class MainWindow;

class LoginPage : public Fl_Group {
public:
    /**
     * @brief Callback function type for successful authentication
     * @param userId The authenticated user's ID
     * @param username The authenticated user's username
     * @param sessionToken The session token for this login
     */
    using AuthCallback = std::function<void(uint64_t userId, const std::string& username, const std::string& sessionToken)>;
    
    LoginPage(int x, int y, int width, int height, UserDatabase* userDatabase);
    ~LoginPage();
    
    /**
     * @brief Set callback for successful authentication
     */
    void setOnAuthenticated(AuthCallback callback);
    
    /**
     * @brief Apply dark/light theme
     */
    void applyTheme(bool isDarkMode);
    
    /**
     * @brief Clear all input fields
     */
    void clearFields();
    
    /**
     * @brief Show error message to user
     */
    void showError(const std::string& message);
    
    /**
     * @brief Show success message to user
     */
    void showSuccess(const std::string& message);
    
    /**
     * @brief Switch between login and register modes
     */
    void setMode(bool isRegisterMode);
    
private:
    UserDatabase* userDatabase;
    AuthCallback onAuthenticatedCallback;
    bool isRegistering;
    
    // UI Components
    Fl_Box* titleLabel;
    Fl_Box* subtitleLabel;
    Fl_Input* usernameInput;
    Fl_Secret_Input* passwordInput;
    Fl_Secret_Input* confirmPasswordInput;  // Only shown in register mode
    Fl_Button* actionButton;                // Login or Register
    Fl_Button* switchModeButton;            // Toggle between modes
    Fl_Box* statusLabel;                    // Error/success messages
    
    // Layout helpers
    void setupLayout();
    void updateButtonLabels();
    
    // Callbacks
    static void onActionButtonClicked(Fl_Widget* widget, void* userdata);
    static void onSwitchModeClicked(Fl_Widget* widget, void* userdata);
    
    // Action handlers
    void performLogin();
    void performRegister();
    
    // Validation
    bool validateLoginInput();
    bool validateRegisterInput();
};

#endif // LOGIN_PAGE_H
