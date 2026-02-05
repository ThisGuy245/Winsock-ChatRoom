#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * @file Protocol.h
 * @brief Application-level protocol for client-server communication
 * 
 * SECURITY MODEL:
 * - All messages have a type field that determines how they're processed
 * - Server NEVER trusts client-provided user IDs in requests
 * - Authentication is required before most operations
 * - All responses include a status code for error handling
 * 
 * WIRE FORMAT:
 * Each message is serialized as JSON for readability and debugging.
 * In production, consider binary format for efficiency.
 * 
 * Message structure:
 * {
 *   "type": "MESSAGE_TYPE",
 *   "requestId": 12345,        // For matching responses to requests
 *   "sessionToken": "...",     // For authenticated requests
 *   "payload": { ... }         // Type-specific data
 * }
 */

#include <string>
#include <cstdint>

namespace Protocol {

//=============================================================================
// MESSAGE TYPES - Client to Server
//=============================================================================

/**
 * Request types that clients can send to the server
 */
enum class RequestType {
    // Authentication
    Register,           // Create new account
    Login,              // Login with credentials
    Logout,             // End session
    
    // Server Management
    CreateServer,       // Create a new chat server
    DeleteServer,       // Delete a server (owner only)
    JoinServer,         // Join an existing server
    LeaveServer,        // Leave a server
    RenameServer,       // Rename a server (owner only)
    GetServerList,      // Get list of user's servers
    GetServerMembers,   // Get members of a server
    
    // Channel Management
    CreateChannel,      // Create channel in a server
    DeleteChannel,      // Delete a channel
    RenameChannel,      // Rename a channel
    GetChannelList,     // Get channels in a server
    JoinChannel,        // Enter a channel (for receiving messages)
    LeaveChannel,       // Exit a channel
    
    // Messaging
    SendMessage,        // Send message to current channel
    SendDirectMessage,  // Send private message to user
    GetMessageHistory,  // Get recent messages from channel
    
    // Friends
    SendFriendRequest,  // Send friend request to user
    AcceptFriendRequest,// Accept incoming request
    DeclineFriendRequest,// Decline incoming request
    RemoveFriend,       // Remove from friends list
    GetFriendList,      // Get list of friends
    GetFriendRequests,  // Get pending requests
    
    // User Info
    GetUserProfile,     // Get user's public info
    UpdateProfile,      // Update own profile
    SearchUsers,        // Search for users by username
    
    // Presence
    Heartbeat           // Keep connection alive, update presence
};

//=============================================================================
// MESSAGE TYPES - Server to Client
//=============================================================================

/**
 * Response/Event types that server sends to clients
 */
enum class ResponseType {
    // Generic
    Success,            // Operation succeeded
    Error,              // Operation failed
    
    // Authentication
    LoginSuccess,       // Login succeeded, includes session token
    RegisterSuccess,    // Registration succeeded
    SessionExpired,     // Session is no longer valid
    
    // Data Responses
    ServerList,         // List of servers
    ChannelList,        // List of channels
    MemberList,         // List of members
    MessageHistory,     // Historical messages
    FriendList,         // List of friends
    FriendRequestList,  // List of pending requests
    UserProfile,        // User profile data
    SearchResults,      // User search results
    
    // Real-time Events (pushed to clients)
    NewMessage,         // New message in channel
    NewDirectMessage,   // New private message
    UserJoined,         // User joined server/channel
    UserLeft,           // User left server/channel
    UserOnline,         // Friend came online
    UserOffline,        // Friend went offline
    FriendRequestReceived, // New friend request
    ServerUpdated,      // Server was renamed
    ChannelUpdated,     // Channel was renamed
    ServerDeleted,      // Server was deleted
    ChannelDeleted,     // Channel was deleted
    Kicked,             // User was kicked from server
};

//=============================================================================
// ERROR CODES
//=============================================================================

enum class ErrorCode {
    None = 0,
    
    // Authentication errors (1xx)
    InvalidCredentials = 100,
    UsernameAlreadyExists = 101,
    InvalidUsername = 102,
    InvalidPassword = 103,
    SessionExpired = 104,
    NotAuthenticated = 105,
    
    // Permission errors (2xx)
    NotAuthorized = 200,
    NotServerOwner = 201,
    NotServerMember = 202,
    
    // Resource errors (3xx)
    ServerNotFound = 300,
    ChannelNotFound = 301,
    UserNotFound = 302,
    MessageNotFound = 303,
    
    // Validation errors (4xx)
    InvalidServerName = 400,
    InvalidChannelName = 401,
    InvalidMessageContent = 402,
    TooManyServers = 403,
    TooManyChannels = 404,
    TooManyFriends = 405,
    
    // Friend errors (5xx)
    AlreadyFriends = 500,
    RequestAlreadySent = 501,
    RequestNotFound = 502,
    CannotFriendSelf = 503,
    
    // Server errors (9xx)
    InternalError = 900,
    RateLimited = 901,
    ServerOverloaded = 902
};

//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

/**
 * @brief Convert RequestType to string for logging/debugging
 */
const char* RequestTypeToString(RequestType type);

/**
 * @brief Convert ResponseType to string for logging/debugging
 */
const char* ResponseTypeToString(ResponseType type);

/**
 * @brief Convert ErrorCode to human-readable message
 */
const char* ErrorCodeToMessage(ErrorCode code);

/**
 * @brief Generate a unique request ID for matching responses
 */
uint32_t GenerateRequestId();

//=============================================================================
// MESSAGE STRUCTURES
// These define the payload format for each message type
//=============================================================================

// Note: In a production system, these would be serialized to JSON or binary.
// For now, we define the logical structure; serialization is in Protocol.cpp

namespace Payloads {

// ---- Authentication ----

struct RegisterRequest {
    std::string username;
    std::string password;  // SECURITY: Transmitted over TLS in production
};

struct LoginRequest {
    std::string username;
    std::string password;
};

struct LoginResponse {
    std::string sessionToken;
    uint64_t userId;
    std::string username;
};

// ---- Server Management ----

struct CreateServerRequest {
    std::string serverName;
};

struct CreateServerResponse {
    uint64_t serverId;
    std::string serverName;
};

struct JoinServerRequest {
    uint64_t serverId;
    // Future: invite code for private servers
};

struct ServerInfo {
    uint64_t serverId;
    std::string serverName;
    uint64_t ownerId;
    std::string ownerName;
    int memberCount;
    int channelCount;
};

// ---- Channel Management ----

struct CreateChannelRequest {
    uint64_t serverId;
    std::string channelName;
};

struct ChannelInfo {
    uint64_t channelId;
    uint64_t serverId;
    std::string channelName;
};

// ---- Messaging ----

struct SendMessageRequest {
    uint64_t channelId;
    std::string content;
};

struct MessageInfo {
    uint64_t messageId;
    uint64_t senderId;
    std::string senderName;
    uint64_t channelId;
    std::string content;
    int64_t timestamp;
};

struct SendDirectMessageRequest {
    uint64_t recipientId;
    std::string content;
};

// ---- Friends ----

struct FriendInfo {
    uint64_t userId;
    std::string username;
    bool isOnline;
};

struct FriendRequestInfo {
    uint64_t requestId;
    uint64_t fromUserId;
    std::string fromUsername;
    int64_t timestamp;
};

// ---- User Info ----

struct UserInfo {
    uint64_t userId;
    std::string username;
    bool isOnline;
    int64_t memberSince;
};

} // namespace Payloads

} // namespace Protocol

#endif // PROTOCOL_H
