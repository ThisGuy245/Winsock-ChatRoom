/**
 * @file LoginPage.cpp
 * @brief Implementation of the Login/Register page
 */

#include "LoginPage.h"
#include <FL/fl_ask.H>

// Color constants
namespace {
    const Fl_Color DARK_BG = fl_rgb_color(45, 45, 48);
    const Fl_Color DARK_INPUT = fl_rgb_color(60, 60, 65);
    const Fl_Color DARK_TEXT = FL_WHITE;
    const Fl_Color LIGHT_BG = FL_WHITE;
    const Fl_Color LIGHT_INPUT = FL_WHITE;
    const Fl_Color LIGHT_TEXT = FL_BLACK;
    const Fl_Color ACCENT_COLOR = fl_rgb_color(88, 101, 242);  // Discord-like blue
    const Fl_Color ERROR_COLOR = fl_rgb_color(237, 66, 69);    // Red
    const Fl_Color SUCCESS_COLOR = fl_rgb_color(87, 242, 135); // Green
}

LoginPage::LoginPage(int x, int y, int width, int height, UserDatabase* userDb)
    : Fl_Group(x, y, width, height)
    , userDatabase(userDb)
    , isRegistering(false)
    , titleLabel(nullptr)
    , subtitleLabel(nullptr)
    , usernameInput(nullptr)
    , passwordInput(nullptr)
    , confirmPasswordInput(nullptr)
    , actionButton(nullptr)
    , switchModeButton(nullptr)
    , statusLabel(nullptr) {
    
    setupLayout();
    end();
}

LoginPage::~LoginPage() {
    // FLTK handles widget cleanup
}

void LoginPage::setupLayout() {
    // Calculate center position for form
    int formWidth = 300;
    int formHeight = 350;
    int formX = x() + (w() - formWidth) / 2;
    int formY = y() + (h() - formHeight) / 2;
    
    int currentY = formY;
    int spacing = 15;
    int inputHeight = 35;
    int buttonHeight = 40;
    
    // Title
    titleLabel = new Fl_Box(formX, currentY, formWidth, 40, "Welcome Back");
    titleLabel->labelsize(24);
    titleLabel->labelfont(FL_BOLD);
    titleLabel->align(FL_ALIGN_CENTER);
    currentY += 40 + spacing;
    
    // Subtitle
    subtitleLabel = new Fl_Box(formX, currentY, formWidth, 20, "Sign in to continue");
    subtitleLabel->labelsize(12);
    subtitleLabel->align(FL_ALIGN_CENTER);
    currentY += 20 + spacing * 2;
    
    // Username input
    usernameInput = new Fl_Input(formX, currentY, formWidth, inputHeight, "");
    usernameInput->textsize(14);
    usernameInput->box(FL_FLAT_BOX);
    // Add placeholder-like label above
    Fl_Box* usernameLabel = new Fl_Box(formX, currentY - 18, formWidth, 18, "Username");
    usernameLabel->labelsize(11);
    usernameLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    currentY += inputHeight + spacing + 10;
    
    // Password input
    passwordInput = new Fl_Secret_Input(formX, currentY, formWidth, inputHeight, "");
    passwordInput->textsize(14);
    passwordInput->box(FL_FLAT_BOX);
    Fl_Box* passwordLabel = new Fl_Box(formX, currentY - 18, formWidth, 18, "Password");
    passwordLabel->labelsize(11);
    passwordLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    currentY += inputHeight + spacing + 10;
    
    // Confirm password (hidden by default)
    confirmPasswordInput = new Fl_Secret_Input(formX, currentY, formWidth, inputHeight, "");
    confirmPasswordInput->textsize(14);
    confirmPasswordInput->box(FL_FLAT_BOX);
    confirmPasswordInput->hide();
    Fl_Box* confirmLabel = new Fl_Box(formX, currentY - 18, formWidth, 18, "Confirm Password");
    confirmLabel->labelsize(11);
    confirmLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    confirmLabel->hide();
    currentY += inputHeight + spacing + 10;
    
    // Status message area
    statusLabel = new Fl_Box(formX, currentY, formWidth, 25, "");
    statusLabel->labelsize(12);
    statusLabel->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    currentY += 25 + spacing;
    
    // Action button (Login/Register)
    actionButton = new Fl_Button(formX, currentY, formWidth, buttonHeight, "Login");
    actionButton->color(ACCENT_COLOR);
    actionButton->labelcolor(FL_WHITE);
    actionButton->labelsize(14);
    actionButton->labelfont(FL_BOLD);
    actionButton->box(FL_FLAT_BOX);
    actionButton->callback(onActionButtonClicked, this);
    currentY += buttonHeight + spacing;
    
    // Switch mode button
    switchModeButton = new Fl_Button(formX, currentY, formWidth, 30, "Need an account? Register");
    switchModeButton->box(FL_NO_BOX);
    switchModeButton->labelcolor(ACCENT_COLOR);
    switchModeButton->labelsize(12);
    switchModeButton->callback(onSwitchModeClicked, this);
}

void LoginPage::setOnAuthenticated(AuthCallback callback) {
    onAuthenticatedCallback = callback;
}

void LoginPage::applyTheme(bool isDarkMode) {
    if (isDarkMode) {
        color(DARK_BG);
        titleLabel->labelcolor(DARK_TEXT);
        subtitleLabel->labelcolor(fl_rgb_color(180, 180, 180));
        
        usernameInput->color(DARK_INPUT);
        usernameInput->textcolor(DARK_TEXT);
        passwordInput->color(DARK_INPUT);
        passwordInput->textcolor(DARK_TEXT);
        confirmPasswordInput->color(DARK_INPUT);
        confirmPasswordInput->textcolor(DARK_TEXT);
    } else {
        color(LIGHT_BG);
        titleLabel->labelcolor(LIGHT_TEXT);
        subtitleLabel->labelcolor(fl_rgb_color(100, 100, 100));
        
        usernameInput->color(LIGHT_INPUT);
        usernameInput->textcolor(LIGHT_TEXT);
        passwordInput->color(LIGHT_INPUT);
        passwordInput->textcolor(LIGHT_TEXT);
        confirmPasswordInput->color(LIGHT_INPUT);
        confirmPasswordInput->textcolor(LIGHT_TEXT);
    }
    
    redraw();
}

void LoginPage::clearFields() {
    usernameInput->value("");
    passwordInput->value("");
    confirmPasswordInput->value("");
    statusLabel->label("");
}

void LoginPage::showError(const std::string& message) {
    statusLabel->labelcolor(ERROR_COLOR);
    statusLabel->copy_label(message.c_str());
    redraw();
}

void LoginPage::showSuccess(const std::string& message) {
    statusLabel->labelcolor(SUCCESS_COLOR);
    statusLabel->copy_label(message.c_str());
    redraw();
}

void LoginPage::setMode(bool isRegisterMode) {
    isRegistering = isRegisterMode;
    updateButtonLabels();
    
    if (isRegistering) {
        titleLabel->label("Create Account");
        subtitleLabel->label("Join the community");
        actionButton->label("Register");
        switchModeButton->label("Already have an account? Login");
        confirmPasswordInput->show();
        // Show the confirm password label too
        Fl_Widget* confirmLabel = child(6);  // Index may vary
        if (confirmLabel) confirmLabel->show();
    } else {
        titleLabel->label("Welcome Back");
        subtitleLabel->label("Sign in to continue");
        actionButton->label("Login");
        switchModeButton->label("Need an account? Register");
        confirmPasswordInput->hide();
        // Hide the confirm password label too
        Fl_Widget* confirmLabel = child(6);  // Index may vary
        if (confirmLabel) confirmLabel->hide();
    }
    
    statusLabel->label("");
    redraw();
}

void LoginPage::updateButtonLabels() {
    // Already handled in setMode
}

void LoginPage::onActionButtonClicked(Fl_Widget* widget, void* userdata) {
    LoginPage* page = static_cast<LoginPage*>(userdata);
    
    if (page->isRegistering) {
        page->performRegister();
    } else {
        page->performLogin();
    }
}

void LoginPage::onSwitchModeClicked(Fl_Widget* widget, void* userdata) {
    LoginPage* page = static_cast<LoginPage*>(userdata);
    page->setMode(!page->isRegistering);
}

void LoginPage::performLogin() {
    if (!validateLoginInput()) {
        return;
    }
    
    std::string username = usernameInput->value();
    std::string password = passwordInput->value();
    
    Models::Session session;
    Protocol::ErrorCode result = userDatabase->AuthenticateUser(username, password, session);
    
    if (result == Protocol::ErrorCode::None) {
        showSuccess("Login successful!");
        
        if (onAuthenticatedCallback) {
            onAuthenticatedCallback(session.userId, username, session.sessionToken);
        }
    } else {
        // Don't reveal whether username or password was wrong
        showError("Invalid username or password");
    }
}

void LoginPage::performRegister() {
    if (!validateRegisterInput()) {
        return;
    }
    
    std::string username = usernameInput->value();
    std::string password = passwordInput->value();
    
    uint64_t userId = 0;
    Protocol::ErrorCode result = userDatabase->RegisterUser(username, password, userId);
    
    switch (result) {
        case Protocol::ErrorCode::None:
            showSuccess("Account created! You can now login.");
            setMode(false);  // Switch to login mode
            break;
            
        case Protocol::ErrorCode::UsernameAlreadyExists:
            showError("Username already taken");
            break;
            
        case Protocol::ErrorCode::InvalidUsername:
            showError("Invalid username format");
            break;
            
        case Protocol::ErrorCode::InvalidPassword:
            showError("Password does not meet requirements");
            break;
            
        default:
            showError("Registration failed. Please try again.");
            break;
    }
}

bool LoginPage::validateLoginInput() {
    std::string username = usernameInput->value();
    std::string password = passwordInput->value();
    
    if (username.empty()) {
        showError("Please enter a username");
        return false;
    }
    
    if (password.empty()) {
        showError("Please enter a password");
        return false;
    }
    
    return true;
}

bool LoginPage::validateRegisterInput() {
    std::string username = usernameInput->value();
    std::string password = passwordInput->value();
    std::string confirmPassword = confirmPasswordInput->value();
    
    if (username.empty()) {
        showError("Please enter a username");
        return false;
    }
    
    if (!Models::User::IsValidUsername(username)) {
        showError("Username: 3-32 chars, letters/numbers/underscore");
        return false;
    }
    
    if (password.empty()) {
        showError("Please enter a password");
        return false;
    }
    
    if (password.length() < Models::MIN_PASSWORD_LENGTH) {
        showError("Password must be at least 8 characters");
        return false;
    }
    
    if (password != confirmPassword) {
        showError("Passwords do not match");
        return false;
    }
    
    return true;
}
