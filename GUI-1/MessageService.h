#ifndef MESSAGE_SERVICE_H
#define MESSAGE_SERVICE_H

/**
 * @file MessageService.h
 * @brief Service for managing per-channel message history
 * 
 * Handles:
 * - Storing messages per channel
 * - Persisting messages to XML file
 * - Loading channel history on demand
 */

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>
#include "Models.h"
#include "pugixml.hpp"

class MessageService {
public:
    /**
     * @brief Constructor - loads existing messages from file
     * @param dataFilePath Path to the XML file for persistence
     */
    explicit MessageService(const std::string& dataFilePath);
    
    /**
     * @brief Destructor - saves messages to file
     */
    ~MessageService();
    
    // =========================================================================
    // MESSAGE OPERATIONS
    // =========================================================================
    
    /**
     * @brief Add a new message to a channel
     * @param channelId The channel to add the message to
     * @param senderId The user who sent the message
     * @param senderName The username (for display)
     * @param content The message content
     * @param type The type of message (default: Text)
     * @return The created message
     */
    Models::Message AddMessage(uint64_t channelId, uint64_t senderId, 
                               const std::string& senderName,
                               const std::string& content,
                               Models::MessageType type = Models::MessageType::Text);
    
    /**
     * @brief Add a system message (join/leave notifications)
     * @param channelId The channel to add the message to
     * @param content The system message content
     * @return The created message
     */
    Models::Message AddSystemMessage(uint64_t channelId, const std::string& content);
    
    /**
     * @brief Get all messages for a channel
     * @param channelId The channel ID
     * @return Vector of messages, sorted by timestamp (oldest first)
     */
    std::vector<Models::Message> GetChannelMessages(uint64_t channelId);
    
    /**
     * @brief Get recent messages for a channel (limited count)
     * @param channelId The channel ID
     * @param limit Maximum number of messages to return
     * @return Vector of most recent messages
     */
    std::vector<Models::Message> GetRecentMessages(uint64_t channelId, size_t limit = 100);
    
    /**
     * @brief Clear all messages in a channel
     * @param channelId The channel to clear
     */
    void ClearChannel(uint64_t channelId);
    
    /**
     * @brief Delete all messages in a server (when server is deleted)
     * @param serverId The server whose channels should be cleared
     * @param channelIds List of channel IDs belonging to the server
     */
    void ClearServerMessages(uint64_t serverId, const std::vector<uint64_t>& channelIds);
    
    // =========================================================================
    // PERSISTENCE
    // =========================================================================
    
    /**
     * @brief Save all messages to file
     */
    void SaveToFile();
    
    /**
     * @brief Load messages from file
     */
    void LoadFromFile();
    
    /**
     * @brief Reload messages from file (to get updates from other instances)
     */
    void ReloadFromFile();
    
    /**
     * @brief Get the formatted display string for a message
     * @param msg The message to format
     * @param senderName The sender's username
     * @return Formatted string like "[12:34] Username: message"
     */
    static std::string FormatMessageForDisplay(const Models::Message& msg, 
                                                const std::string& senderName);

private:
    std::string dataFilePath;
    
    // Channel ID -> Vector of messages
    std::map<uint64_t, std::vector<Models::Message>> channelMessages;
    
    // Message ID -> Sender name (for display purposes)
    std::map<uint64_t, std::string> senderNames;
    
    // Thread safety
    mutable std::mutex serviceMutex;
    
    // Maximum messages to store per channel (to prevent memory issues)
    static constexpr size_t MAX_MESSAGES_PER_CHANNEL = 1000;
};

#endif // MESSAGE_SERVICE_H
