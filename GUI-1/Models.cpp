/**
 * @file Models.cpp
 * @brief Implementation of core data models
 */

#include "Models.h"
#include <algorithm>
#include <cctype>
#include <random>
#include <chrono>

namespace Models {

//=============================================================================
// ID GENERATION
//=============================================================================

uint64_t GenerateUniqueId() {
    static std::random_device randomDevice;
    static std::mt19937_64 generator(randomDevice());
    static std::uniform_int_distribution<uint16_t> distribution(0, UINT16_MAX);
    
    // Get current timestamp in milliseconds
    auto now = std::chrono::system_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    // Combine: [48-bit timestamp][16-bit random]
    uint64_t timestamp = static_cast<uint64_t>(milliseconds) & 0xFFFFFFFFFFFF;
    uint64_t random = distribution(generator);
    
    return (timestamp << 16) | random;
}

//=============================================================================
// USER IMPLEMENTATION
//=============================================================================

User::User() 
    : userId(0)
    , username("")
    , createdAt(0)
    , lastLoginAt(0)
    , isOnline(false) {
}

User::User(uint64_t id, const std::string& name)
    : userId(id)
    , username(name)
    , createdAt(std::time(nullptr))
    , lastLoginAt(0)
    , isOnline(false) {
}

bool User::IsValidUsername(const std::string& username) {
    // Check length
    if (username.empty() || username.length() > MAX_USERNAME_LENGTH) {
        return false;
    }
    
    // Must start with a letter
    if (!std::isalpha(static_cast<unsigned char>(username[0]))) {
        return false;
    }
    
    // Allow only alphanumeric and underscores
    for (char c : username) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    
    // Reserved names (case-insensitive)
    std::string lowerName = username;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    const std::vector<std::string> reserved = {
        "admin", "system", "server", "moderator", "mod", "root", "null"
    };
    
    for (const auto& name : reserved) {
        if (lowerName == name) {
            return false;
        }
    }
    
    return true;
}

//=============================================================================
// FRIEND REQUEST IMPLEMENTATION
//=============================================================================

FriendRequest::FriendRequest()
    : requestId(0)
    , senderId(0)
    , receiverId(0)
    , status(Status::Pending)
    , createdAt(0)
    , respondedAt(0) {
}

FriendRequest::FriendRequest(uint64_t id, uint64_t sender, uint64_t receiver)
    : requestId(id)
    , senderId(sender)
    , receiverId(receiver)
    , status(Status::Pending)
    , createdAt(std::time(nullptr))
    , respondedAt(0) {
}

//=============================================================================
// CHAT SERVER IMPLEMENTATION
//=============================================================================

ChatServer::ChatServer()
    : serverId(0)
    , serverName("")
    , ownerId(0)
    , createdAt(0)
    , hostIpAddress("")
    , hostPort(DEFAULT_PORT)
    , isOnline(false) {
}

ChatServer::ChatServer(uint64_t id, const std::string& name, uint64_t owner)
    : serverId(id)
    , serverName(name)
    , ownerId(owner)
    , createdAt(std::time(nullptr))
    , hostIpAddress("")
    , hostPort(DEFAULT_PORT)
    , isOnline(false) {
}

bool ChatServer::IsValidServerName(const std::string& name) {
    if (name.empty() || name.length() > MAX_SERVER_NAME_LENGTH) {
        return false;
    }
    
    // Allow letters, numbers, spaces, hyphens, underscores
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && 
            c != ' ' && c != '-' && c != '_') {
            return false;
        }
    }
    
    // No leading/trailing whitespace
    if (std::isspace(static_cast<unsigned char>(name.front())) ||
        std::isspace(static_cast<unsigned char>(name.back()))) {
        return false;
    }
    
    return true;
}

bool ChatServer::IsOwner(uint64_t userId) const {
    return userId == ownerId;
}

bool ChatServer::IsMember(uint64_t userId) const {
    return std::find(memberIds.begin(), memberIds.end(), userId) != memberIds.end();
}

//=============================================================================
// CHANNEL IMPLEMENTATION
//=============================================================================

Channel::Channel()
    : channelId(0)
    , serverId(0)
    , channelName("")
    , createdAt(0) {
}

Channel::Channel(uint64_t id, uint64_t server, const std::string& name)
    : channelId(id)
    , serverId(server)
    , channelName(name)
    , createdAt(std::time(nullptr)) {
}

bool Channel::IsValidChannelName(const std::string& name) {
    if (name.empty() || name.length() > MAX_CHANNEL_NAME_LENGTH) {
        return false;
    }
    
    // Channel names: lowercase, numbers, hyphens only (like Discord)
    for (char c : name) {
        if (!std::islower(static_cast<unsigned char>(c)) && 
            !std::isdigit(static_cast<unsigned char>(c)) && 
            c != '-') {
            return false;
        }
    }
    
    // No leading/trailing hyphens
    if (name.front() == '-' || name.back() == '-') {
        return false;
    }
    
    return true;
}

//=============================================================================
// MESSAGE IMPLEMENTATION
//=============================================================================

Message::Message()
    : messageId(0)
    , senderId(0)
    , channelId(0)
    , recipientId(0)
    , type(MessageType::Text)
    , content("")
    , timestamp(0)
    , isEdited(false) {
}

Message::Message(uint64_t sender, uint64_t channel, const std::string& text)
    : messageId(GenerateUniqueId())
    , senderId(sender)
    , channelId(channel)
    , recipientId(0)
    , type(MessageType::Text)
    , content(text)
    , timestamp(std::time(nullptr))
    , isEdited(false) {
}

Message Message::CreateTextMessage(uint64_t sender, uint64_t channel, const std::string& text) {
    Message message(sender, channel, text);
    message.type = MessageType::Text;
    return message;
}

Message Message::CreateSystemMessage(uint64_t channel, const std::string& text) {
    Message message;
    message.messageId = GenerateUniqueId();
    message.senderId = 0;  // System messages have no sender
    message.channelId = channel;
    message.type = MessageType::System;
    message.content = text;
    message.timestamp = std::time(nullptr);
    return message;
}

Message Message::CreateDirectMessage(uint64_t sender, uint64_t recipient, const std::string& text) {
    Message message;
    message.messageId = GenerateUniqueId();
    message.senderId = sender;
    message.channelId = 0;  // DMs don't belong to a channel
    message.recipientId = recipient;
    message.type = MessageType::DirectMessage;
    message.content = text;
    message.timestamp = std::time(nullptr);
    return message;
}

bool Message::IsValidContent(const std::string& content) {
    if (content.empty() || content.length() > MAX_MESSAGE_LENGTH) {
        return false;
    }
    
    // Check for control characters (except newlines and tabs)
    for (char c : content) {
        if (std::iscntrl(static_cast<unsigned char>(c)) && c != '\n' && c != '\t' && c != '\r') {
            return false;
        }
    }
    
    return true;
}

//=============================================================================
// SESSION IMPLEMENTATION
//=============================================================================

Session::Session()
    : sessionToken("")
    , userId(0)
    , createdAt(0)
    , expiresAt(0)
    , lastActivityAt(0)
    , currentServerId(0)
    , currentChannelId(0) {
}

Session::Session(uint64_t user, const std::string& token)
    : sessionToken(token)
    , userId(user)
    , createdAt(std::time(nullptr))
    , expiresAt(std::time(nullptr) + SESSION_LIFETIME_SECONDS)
    , lastActivityAt(std::time(nullptr))
    , currentServerId(0)
    , currentChannelId(0) {
}

bool Session::IsExpired() const {
    return std::time(nullptr) > expiresAt;
}

void Session::UpdateActivity() {
    lastActivityAt = std::time(nullptr);
    // Extend expiry on activity
    expiresAt = lastActivityAt + SESSION_LIFETIME_SECONDS;
}

} // namespace Models
