#ifndef MODELS_H
#define MODELS_H

/**
 * @file Models.h
 * @brief Core data models for the Discord-like chat application
 * 
 * SECURITY PRINCIPLES:
 * - All IDs use uint64_t to prevent collision attacks
 * - Timestamps use UTC to prevent timezone manipulation
 * - Password hashes are NEVER stored in these models (see UserDatabase)
 * - All string fields have maximum lengths defined
 * 
 * DATA FLOW:
 * User -> joins -> Server -> contains -> Channels -> contain -> Messages
 * User -> has -> Friends (bidirectional relationship)
 */

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

namespace Models {

//=============================================================================
// CONSTANTS - Maximum field lengths for security and consistency
//=============================================================================

constexpr size_t MAX_USERNAME_LENGTH = 32;
constexpr size_t MAX_SERVER_NAME_LENGTH = 64;
constexpr size_t MAX_CHANNEL_NAME_LENGTH = 32;
constexpr size_t MAX_MESSAGE_LENGTH = 2000;  // Like Discord
constexpr size_t MAX_CHANNELS_PER_SERVER = 50;
constexpr size_t MAX_SERVERS_PER_USER = 100;
constexpr size_t MAX_FRIENDS_PER_USER = 1000;
constexpr size_t MIN_PASSWORD_LENGTH = 8;

//=============================================================================
// ID GENERATION
//=============================================================================

/**
 * @brief Generate a unique ID based on timestamp and random component
 * 
 * Format: [48-bit timestamp][16-bit random]
 * This provides:
 * - Chronological ordering (useful for messages)
 * - Collision resistance
 * - No need for central ID server
 */
uint64_t GenerateUniqueId();

//=============================================================================
// USER MODEL
//=============================================================================

/**
 * @brief Represents a user account in the system
 * 
 * SECURITY NOTE: This model does NOT contain password information.
 * Password hashes are stored separately in UserDatabase for isolation.
 */
struct User {
    uint64_t userId;
    std::string username;
    std::time_t createdAt;
    std::time_t lastLoginAt;
    bool isOnline;
    
    // Relationships (stored as IDs for lazy loading)
    std::vector<uint64_t> serverIds;       // Servers this user has joined
    std::vector<uint64_t> friendIds;       // Confirmed friends
    std::vector<uint64_t> pendingFriendRequestIds;  // Incoming requests
    std::vector<uint64_t> sentFriendRequestIds;     // Outgoing requests
    
    User();
    User(uint64_t id, const std::string& name);
    
    // Validation
    static bool IsValidUsername(const std::string& username);
};

//=============================================================================
// FRIEND REQUEST MODEL
//=============================================================================

/**
 * @brief Represents a friend request between two users
 */
struct FriendRequest {
    // Status enum inside struct for cleaner access
    enum class Status {
        Pending,
        Accepted,
        Declined,
        Cancelled
    };
    
    uint64_t requestId;
    uint64_t senderId;      // User who sent the request
    uint64_t receiverId;    // User who receives the request
    Status status;
    std::time_t createdAt;
    std::time_t respondedAt;
    
    FriendRequest();
    FriendRequest(uint64_t id, uint64_t sender, uint64_t receiver);
};

// Keep alias for backward compatibility
using FriendRequestStatus = FriendRequest::Status;

//=============================================================================
// SERVER MODEL (like Discord Server/Guild)
//=============================================================================

/**
 * @brief Represents a chat server that contains channels
 * 
 * A "Server" is like a Discord server - a container for multiple channels
 * that users can join. The owner has full permissions.
 * 
 * NETWORKING: Each server has an IP and port for real-time chat.
 * The owner's machine hosts the actual chat server.
 */
struct ChatServer {
    uint64_t serverId;
    std::string serverName;
    uint64_t ownerId;           // User who created the server
    std::time_t createdAt;
    
    // Networking info (for real-time chat)
    std::string hostIpAddress;  // IP address of the host
    uint16_t hostPort;          // Port number (default: 54000)
    bool isOnline;              // Is the server currently hosted/active
    
    // Relationships
    std::vector<uint64_t> channelIds;   // Channels in this server
    std::vector<uint64_t> memberIds;    // Users who have joined
    
    ChatServer();
    ChatServer(uint64_t id, const std::string& name, uint64_t owner);
    
    // Validation
    static bool IsValidServerName(const std::string& name);
    
    // Permission checks
    bool IsOwner(uint64_t userId) const;
    bool IsMember(uint64_t userId) const;
    
    // Default port for chat servers
    static constexpr uint16_t DEFAULT_PORT = 54000;
};

//=============================================================================
// CHANNEL MODEL
//=============================================================================

/**
 * @brief Represents a chat channel within a server
 * 
 * Channels are where actual conversations happen.
 * Each server has at least one default channel ("general").
 */
struct Channel {
    uint64_t channelId;
    uint64_t serverId;          // Parent server
    std::string channelName;
    std::time_t createdAt;
    
    // Note: Message history is NOT stored here for memory efficiency
    // Messages are loaded on-demand from MessageStore
    
    Channel();
    Channel(uint64_t id, uint64_t server, const std::string& name);
    
    // Validation
    static bool IsValidChannelName(const std::string& name);
};

//=============================================================================
// MESSAGE MODEL
//=============================================================================

enum class MessageType {
    Text,           // Regular chat message
    System,         // System notification (join/leave/etc)
    DirectMessage   // Private message between users
};

/**
 * @brief Represents a single chat message
 * 
 * SECURITY NOTE: Message content is stored as-is.
 * Sanitization for display happens at the UI layer.
 * The senderId is verified by the server, not trusted from client.
 */
struct Message {
    uint64_t messageId;
    uint64_t senderId;          // User who sent this
    uint64_t channelId;         // Channel it was sent to (0 for DMs)
    uint64_t recipientId;       // For DMs only, 0 for channel messages
    MessageType type;
    std::string content;
    std::time_t timestamp;
    bool isEdited;
    
    Message();
    Message(uint64_t sender, uint64_t channel, const std::string& text);
    
    // Factory methods for different message types
    static Message CreateTextMessage(uint64_t sender, uint64_t channel, const std::string& text);
    static Message CreateSystemMessage(uint64_t channel, const std::string& text);
    static Message CreateDirectMessage(uint64_t sender, uint64_t recipient, const std::string& text);
    
    // Validation
    static bool IsValidContent(const std::string& content);
};

//=============================================================================
// SESSION MODEL (for tracking logged-in users)
//=============================================================================

/**
 * @brief Represents an active user session
 * 
 * SECURITY: Sessions have expiry times and are tied to specific connections.
 * Session tokens should be unpredictable (generated securely).
 */
struct Session {
    std::string sessionToken;   // Secure random token
    uint64_t userId;
    std::time_t createdAt;
    std::time_t expiresAt;
    std::time_t lastActivityAt;
    
    // Current location in the app
    uint64_t currentServerId;   // 0 if not in a server
    uint64_t currentChannelId;  // 0 if not in a channel
    
    Session();
    Session(uint64_t user, const std::string& token);
    
    bool IsExpired() const;
    void UpdateActivity();
    
    // Session lifetime in seconds
    static constexpr int SESSION_LIFETIME_SECONDS = 24 * 60 * 60;  // 24 hours
};

} // namespace Models

#endif // MODELS_H
