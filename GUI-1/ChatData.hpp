#ifndef CHATDATA_HPP
#define CHATDATA_HPP

#include <string>
#include <vector>
#include <ctime>
#include "pugixml.hpp"

// Struct to hold user information
struct User {
    std::string name;
    std::string last_login;
};

// Struct to hold message information
struct Message {
    std::string user;
    std::string time;
    std::string content;
};

// Class to manage chat data
class ChatData {
public:
    // Loads the chat data from the XML file
    void load(const std::string& filename);

    // Saves the current chat data to the XML file
    void save(const std::string& filename);

    // Adds a new user
    void addUser(const std::string& username);

    // Adds a new message
    void addMessage(const std::string& user, const std::string& content);

    // Prints current data to the console (for debugging)
    void printData() const;

    std::vector<Message> getMessages() const;

private:
    std::vector<User> users;
    std::vector<Message> messages;
};

#endif
