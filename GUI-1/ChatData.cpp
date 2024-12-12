#include "ChatData.hpp"
#include <iostream>
#include <sstream>

// Load chat data from XML file
void ChatData::load(const std::string& filename) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());

    if (!result) {
        std::cerr << "XML [" << filename << "] parsed with errors, error: " << result.description() << std::endl;
        return;
    }

    // Load users
    pugi::xml_node usersNode = doc.child("chat").child("users");
    for (pugi::xml_node userNode : usersNode.children("user")) {
        User user;
        user.name = userNode.attribute("name").value();
        users.push_back(user);
    }

    // Load messages
    pugi::xml_node messagesNode = doc.child("chat").child("messages");
    for (pugi::xml_node messageNode : messagesNode.children("message")) {
        Message msg;
        msg.user = messageNode.attribute("user").value();
        msg.content = messageNode.child_value();
        messages.push_back(msg);
    }
}

// Save chat data to XML file
void ChatData::save(const std::string& filename) {
    pugi::xml_document doc;

    // Create root element <chat>
    pugi::xml_node chatNode = doc.append_child("chat");

    // Add users
    pugi::xml_node usersNode = chatNode.append_child("users");
    for (const auto& user : users) {
        pugi::xml_node userNode = usersNode.append_child("user");
        userNode.append_attribute("name") = user.name.c_str();
    }

    // Add messages
    pugi::xml_node messagesNode = chatNode.append_child("messages");
    for (const auto& msg : messages) {
        pugi::xml_node messageNode = messagesNode.append_child("message");
        messageNode.append_attribute("user") = msg.user.c_str();
        messageNode.append_child(pugi::node_pcdata).set_value(msg.content.c_str());
    }

    // Save to file
    if (!doc.save_file(filename.c_str())) {
        std::cerr << "Error saving XML file!" << std::endl;
    }
}

// Add a new user to the data
void ChatData::addUser(const std::string& username) {
    for (const auto& user : users) {
        if (user.name == username) {
            return; // User already exists
        }
    }
    User user;
    user.name = username;
    users.push_back(user);
}

// Add a new message to the data
void ChatData::addMessage(const std::string& user, const std::string& content) {
    Message msg;
    msg.user = user;
    msg.content = content;
    messages.push_back(msg);
}

// Print all users and messages for debugging
void ChatData::printData() const {
    std::cout << "Users:\n";
    for (const auto& user : users) {
        std::cout << "  Name: " << user.name << std::endl;
    }
    std::cout << "\nMessages:\n";
    for (const auto& msg : messages) {
        std::cout << "  User: " << msg.user << ", Message: " << msg.content << std::endl;
    }
}

std::vector<Message> ChatData::getMessages() const {
    return messages;
}