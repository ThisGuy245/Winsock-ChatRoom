#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

/**
 * @file ServerManager.h
 * @brief Manages chat servers, channels, and their relationships
 * 
 * ARCHITECTURE:
 * - ChatServer contains multiple Channels
 * - Users can be members of multiple Servers
 * - Only server owners can create/delete channels, rename server
 * - All operations are validated before execution
 * 
 * SECURITY:
 * - All public methods validate permissions before acting
 * - User IDs are verified against sessions, never trusted from client
 * - Proper cleanup when servers/channels are deleted
 */

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include "Models.h"
#include "Protocol.h"
#include "pugixml.hpp"

// Forward declaration
class UserDatabase;

class ServerManager {
public:
    /**
     * @brief Initialize with database path and user database reference
     * @param databasePath Path to XML storage file
     * @param userDb Reference to user database for cross-referencing
     */
    ServerManager(const std::string& databasePath, UserDatabase& userDb);
    
    ~ServerManager();
    
    // =========================================================================
    // SERVER OPERATIONS
    // =========================================================================
    
    /**
     * @brief Create a new chat server
     * 
     * Creates a server with a default "general" channel.
     * The creating user becomes the owner and is automatically added as member.
     * 
     * @param serverName Name of the server
     * @param ownerId User ID of the creator (becomes owner)
     * @param outServer Output: created server info
     * @return Error code (None on success)
     */
    Protocol::ErrorCode CreateServer(
        const std::string& serverName,
        uint64_t ownerId,
        Models::ChatServer& outServer
    );
    
    /**
     * @brief Delete a server (owner only)
     * 
     * Removes the server and all its channels.
     * Notifies all members they've been removed.
     * 
     * @param serverId Server to delete
     * @param requesterId User requesting deletion
     * @return Error code (None on success)
     */
    Protocol::ErrorCode DeleteServer(uint64_t serverId, uint64_t requesterId);
    
    /**
     * @brief Set server's network info (IP and port)
     * @param serverId Server to update
     * @param ipAddress The host's IP address
     * @param port The port number
     */
    void SetServerNetworkInfo(uint64_t serverId, const std::string& ipAddress, uint16_t port);
    
    /**
     * @brief Set server online/offline status
     * @param serverId Server to update
     * @param isOnline True if server is currently hosted
     */
    void SetServerOnlineStatus(uint64_t serverId, bool isOnline);
    
    /**
     * @brief Rename a server (owner only)
     * @param serverId Server to rename
     * @param newName New server name
     * @param requesterId User requesting rename
     * @return Error code (None on success)
     */
    Protocol::ErrorCode RenameServer(
        uint64_t serverId,
        const std::string& newName,
        uint64_t requesterId
    );
    
    /**
     * @brief Join a server
     * 
     * Adds user to server's member list.
     * User is added to their server list.
     * 
     * @param serverId Server to join
     * @param userId User joining
     * @return Error code (None on success)
     */
    Protocol::ErrorCode JoinServer(uint64_t serverId, uint64_t userId);
    
    /**
     * @brief Leave a server
     * 
     * Removes user from server's member list.
     * If owner leaves, ownership transfers to oldest member or server is deleted.
     * 
     * @param serverId Server to leave
     * @param userId User leaving
     * @return Error code (None on success)
     */
    Protocol::ErrorCode LeaveServer(uint64_t serverId, uint64_t userId);
    
    /**
     * @brief Get server info by ID
     * @param serverId Server ID
     * @param outServer Output: server data
     * @return True if found
     */
    bool GetServer(uint64_t serverId, Models::ChatServer& outServer);
    
    /**
     * @brief Get all servers a user is a member of
     * @param userId User ID
     * @return Vector of servers
     */
    std::vector<Models::ChatServer> GetUserServers(uint64_t userId);
    
    /**
     * @brief Get all members of a server
     * @param serverId Server ID
     * @return Vector of member user IDs
     */
    std::vector<uint64_t> GetServerMembers(uint64_t serverId);
    
    /**
     * @brief Search for public servers by name
     * @param searchTerm Search string
     * @param maxResults Maximum results to return
     * @return Vector of matching servers
     */
    std::vector<Models::ChatServer> SearchServers(
        const std::string& searchTerm,
        size_t maxResults = 20
    );
    
    // =========================================================================
    // CHANNEL OPERATIONS
    // =========================================================================
    
    /**
     * @brief Create a channel in a server (owner only)
     * @param serverId Server to add channel to
     * @param channelName Name of new channel
     * @param requesterId User requesting creation
     * @param outChannel Output: created channel
     * @return Error code (None on success)
     */
    Protocol::ErrorCode CreateChannel(
        uint64_t serverId,
        const std::string& channelName,
        uint64_t requesterId,
        Models::Channel& outChannel
    );
    
    /**
     * @brief Delete a channel (owner only)
     * @param channelId Channel to delete
     * @param requesterId User requesting deletion
     * @return Error code (None on success)
     */
    Protocol::ErrorCode DeleteChannel(uint64_t channelId, uint64_t requesterId);
    
    /**
     * @brief Rename a channel (owner only)
     * @param channelId Channel to rename
     * @param newName New channel name
     * @param requesterId User requesting rename
     * @return Error code (None on success)
     */
    Protocol::ErrorCode RenameChannel(
        uint64_t channelId,
        const std::string& newName,
        uint64_t requesterId
    );
    
    /**
     * @brief Get channel info by ID
     * @param channelId Channel ID
     * @param outChannel Output: channel data
     * @return True if found
     */
    bool GetChannel(uint64_t channelId, Models::Channel& outChannel);
    
    /**
     * @brief Get all channels in a server
     * @param serverId Server ID
     * @return Vector of channels
     */
    std::vector<Models::Channel> GetServerChannels(uint64_t serverId);
    
    /**
     * @brief Get the default (first) channel of a server
     * @param serverId Server ID
     * @param outChannel Output: channel data
     * @return True if found
     */
    bool GetDefaultChannel(uint64_t serverId, Models::Channel& outChannel);
    
    // =========================================================================
    // PERMISSION CHECKS
    // =========================================================================
    
    /**
     * @brief Check if user is owner of server
     */
    bool IsServerOwner(uint64_t serverId, uint64_t userId);
    
    /**
     * @brief Check if user is member of server
     */
    bool IsServerMember(uint64_t serverId, uint64_t userId);
    
    /**
     * @brief Check if user can access a channel
     * (Must be member of parent server)
     */
    bool CanAccessChannel(uint64_t channelId, uint64_t userId);
    
    // =========================================================================
    // PERSISTENCE
    // =========================================================================
    
    bool SaveToFile();
    bool LoadFromFile();
    
private:
    std::string databaseFilePath;
    UserDatabase& userDatabase;
    
    // In-memory storage
    std::map<uint64_t, Models::ChatServer> serversById;
    std::map<uint64_t, Models::Channel> channelsById;
    
    // Thread safety
    mutable std::mutex managerMutex;
    
    // Helper to get server by channel
    bool GetServerByChannel(uint64_t channelId, Models::ChatServer& outServer);
};

#endif // SERVER_MANAGER_H
