#include "MessageService.h"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

MessageService::MessageService(const std::string& dataFilePath)
    : dataFilePath(dataFilePath) {
    LoadFromFile();
}

MessageService::~MessageService() {
    SaveToFile();
}

Models::Message MessageService::AddMessage(uint64_t channelId, uint64_t senderId,
                                           const std::string& senderName,
                                           const std::string& content,
                                           Models::MessageType type) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    Models::Message msg;
    msg.messageId = Models::GenerateUniqueId();
    msg.channelId = channelId;
    msg.senderId = senderId;
    msg.content = content;
    msg.type = type;
    msg.timestamp = std::time(nullptr);
    msg.isEdited = false;
    msg.recipientId = 0;
    
    // Store sender name for display
    senderNames[msg.messageId] = senderName;
    
    // Add to channel
    auto& messages = channelMessages[channelId];
    messages.push_back(msg);
    
    // Trim if too many messages
    if (messages.size() > MAX_MESSAGES_PER_CHANNEL) {
        // Remove oldest messages
        size_t toRemove = messages.size() - MAX_MESSAGES_PER_CHANNEL;
        for (size_t i = 0; i < toRemove; ++i) {
            senderNames.erase(messages[i].messageId);
        }
        messages.erase(messages.begin(), messages.begin() + toRemove);
    }
    
    // Save immediately so other users can see messages
    SaveToFile();
    
    return msg;
}

Models::Message MessageService::AddSystemMessage(uint64_t channelId, const std::string& content) {
    return AddMessage(channelId, 0, "[SERVER]", content, Models::MessageType::System);
}

std::vector<Models::Message> MessageService::GetChannelMessages(uint64_t channelId) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    auto it = channelMessages.find(channelId);
    if (it != channelMessages.end()) {
        return it->second;
    }
    return {};
}

std::vector<Models::Message> MessageService::GetRecentMessages(uint64_t channelId, size_t limit) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    auto it = channelMessages.find(channelId);
    if (it == channelMessages.end()) {
        return {};
    }
    
    const auto& messages = it->second;
    if (messages.size() <= limit) {
        return messages;
    }
    
    // Return the last 'limit' messages
    return std::vector<Models::Message>(messages.end() - limit, messages.end());
}

void MessageService::ClearChannel(uint64_t channelId) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    auto it = channelMessages.find(channelId);
    if (it != channelMessages.end()) {
        // Remove sender names for these messages
        for (const auto& msg : it->second) {
            senderNames.erase(msg.messageId);
        }
        channelMessages.erase(it);
    }
    
    SaveToFile();
}

void MessageService::ClearServerMessages(uint64_t serverId, const std::vector<uint64_t>& channelIds) {
    for (uint64_t channelId : channelIds) {
        ClearChannel(channelId);
    }
}

void MessageService::SaveToFile() {
    // Note: Caller should NOT hold the lock when calling this from destructor
    // For internal calls, lock is already held
    
    pugi::xml_document doc;
    auto root = doc.append_child("MessageHistory");
    
    for (const auto& [channelId, messages] : channelMessages) {
        auto channelNode = root.append_child("Channel");
        channelNode.append_attribute("id") = channelId;
        
        for (const auto& msg : messages) {
            auto msgNode = channelNode.append_child("Message");
            msgNode.append_attribute("id") = msg.messageId;
            msgNode.append_attribute("senderId") = msg.senderId;
            msgNode.append_attribute("timestamp") = static_cast<long long>(msg.timestamp);
            msgNode.append_attribute("type") = static_cast<int>(msg.type);
            msgNode.append_attribute("edited") = msg.isEdited;
            
            // Store sender name
            auto nameIt = senderNames.find(msg.messageId);
            if (nameIt != senderNames.end()) {
                msgNode.append_attribute("senderName") = nameIt->second.c_str();
            }
            
            msgNode.text().set(msg.content.c_str());
        }
    }
    
    if (!doc.save_file(dataFilePath.c_str())) {
        printf("[MSG] Failed to save message history to %s\n", dataFilePath.c_str());
    } else {
        printf("[MSG] Saved message history\n");
    }
}

void MessageService::LoadFromFile() {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(dataFilePath.c_str());
    
    if (!result) {
        printf("[MSG] No existing message history found, starting fresh\n");
        return;
    }
    
    auto root = doc.child("MessageHistory");
    
    for (auto channelNode : root.children("Channel")) {
        uint64_t channelId = channelNode.attribute("id").as_ullong();
        
        for (auto msgNode : channelNode.children("Message")) {
            Models::Message msg;
            msg.messageId = msgNode.attribute("id").as_ullong();
            msg.channelId = channelId;
            msg.senderId = msgNode.attribute("senderId").as_ullong();
            msg.timestamp = static_cast<std::time_t>(msgNode.attribute("timestamp").as_llong());
            msg.type = static_cast<Models::MessageType>(msgNode.attribute("type").as_int());
            msg.isEdited = msgNode.attribute("edited").as_bool();
            msg.content = msgNode.text().as_string();
            msg.recipientId = 0;
            
            // Load sender name
            std::string senderName = msgNode.attribute("senderName").as_string();
            if (!senderName.empty()) {
                senderNames[msg.messageId] = senderName;
            }
            
            channelMessages[channelId].push_back(msg);
        }
    }
    
    printf("[MSG] Loaded message history for %zu channels\n", channelMessages.size());
}

void MessageService::ReloadFromFile() {
    // Clear current data and reload from file
    // This is used to get updates from other instances
    {
        std::lock_guard<std::mutex> lock(serviceMutex);
        channelMessages.clear();
        senderNames.clear();
    }
    
    // LoadFromFile acquires its own lock
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(dataFilePath.c_str());
    
    if (!result) {
        printf("[MSG] Failed to reload message history\n");
        return;
    }
    
    std::lock_guard<std::mutex> lock(serviceMutex);
    auto root = doc.child("MessageHistory");
    
    for (auto channelNode : root.children("Channel")) {
        uint64_t channelId = channelNode.attribute("id").as_ullong();
        
        for (auto msgNode : channelNode.children("Message")) {
            Models::Message msg;
            msg.messageId = msgNode.attribute("id").as_ullong();
            msg.channelId = channelId;
            msg.senderId = msgNode.attribute("senderId").as_ullong();
            msg.timestamp = static_cast<std::time_t>(msgNode.attribute("timestamp").as_llong());
            msg.type = static_cast<Models::MessageType>(msgNode.attribute("type").as_int());
            msg.isEdited = msgNode.attribute("edited").as_bool();
            msg.content = msgNode.text().as_string();
            msg.recipientId = 0;
            
            std::string senderName = msgNode.attribute("senderName").as_string();
            if (!senderName.empty()) {
                senderNames[msg.messageId] = senderName;
            }
            
            channelMessages[channelId].push_back(msg);
        }
    }
    
    printf("[MSG] Reloaded message history for %zu channels\n", channelMessages.size());
}

std::string MessageService::FormatMessageForDisplay(const Models::Message& msg,
                                                     const std::string& senderName) {
    // Format timestamp
    std::tm tm;
    localtime_s(&tm, &msg.timestamp);
    std::ostringstream oss;
    
    if (msg.type == Models::MessageType::System) {
        oss << "[" << std::setfill('0') << std::setw(2) << tm.tm_hour
            << ":" << std::setw(2) << tm.tm_min << "] "
            << msg.content;
    } else {
        oss << "[" << std::setfill('0') << std::setw(2) << tm.tm_hour
            << ":" << std::setw(2) << tm.tm_min << "] "
            << senderName << ": " << msg.content;
    }
    
    return oss.str();
}
