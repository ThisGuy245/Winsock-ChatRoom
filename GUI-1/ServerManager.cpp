/**
 * @file ServerManager.cpp
 * @brief Implementation of server and channel management
 */

#include "ServerManager.h"
#include "UserDatabase.h"
#include <algorithm>

//=============================================================================
// CONSTRUCTOR / DESTRUCTOR
//=============================================================================

ServerManager::ServerManager(const std::string& databasePath, UserDatabase& userDb)
    : databaseFilePath(databasePath)
    , userDatabase(userDb) {
    LoadFromFile();
}

ServerManager::~ServerManager() {
    SaveToFile();
}

//=============================================================================
// SERVER OPERATIONS
//=============================================================================

Protocol::ErrorCode ServerManager::CreateServer(
    const std::string& serverName,
    uint64_t ownerId,
    Models::ChatServer& outServer
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    // Validate server name
    if (!Models::ChatServer::IsValidServerName(serverName)) {
        return Protocol::ErrorCode::InvalidServerName;
    }
    
    // Check user's server limit
    std::vector<uint64_t> userServers = userDatabase.GetUserServers(ownerId);
    if (userServers.size() >= Models::MAX_SERVERS_PER_USER) {
        return Protocol::ErrorCode::TooManyServers;
    }
    
    // Create server
    uint64_t serverId = Models::GenerateUniqueId();
    Models::ChatServer server(serverId, serverName, ownerId);
    
    // Add owner as first member
    server.memberIds.push_back(ownerId);
    
    // Create default "general" channel
    uint64_t channelId = Models::GenerateUniqueId();
    Models::Channel defaultChannel(channelId, serverId, "general");
    
    server.channelIds.push_back(channelId);
    
    // Store
    serversById[serverId] = server;
    channelsById[channelId] = defaultChannel;
    
    // Update user's server list
    userDatabase.AddUserToServer(ownerId, serverId);
    
    outServer = server;
    
    SaveToFile();
    
    printf("[SERVER] Created server '%s' (ID: %llu) by user %llu\n", 
           serverName.c_str(), serverId, ownerId);
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode ServerManager::DeleteServer(uint64_t serverId, uint64_t requesterId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return Protocol::ErrorCode::ServerNotFound;
    }
    
    // Only owner can delete
    if (it->second.ownerId != requesterId) {
        return Protocol::ErrorCode::NotServerOwner;
    }
    
    Models::ChatServer& server = it->second;
    
    // Remove all channels
    for (uint64_t channelId : server.channelIds) {
        channelsById.erase(channelId);
    }
    
    // Remove server from all members' server lists
    for (uint64_t memberId : server.memberIds) {
        userDatabase.RemoveUserFromServer(memberId, serverId);
    }
    
    printf("[SERVER] Deleted server '%s' (ID: %llu)\n", 
           server.serverName.c_str(), serverId);
    
    // Delete server
    serversById.erase(it);
    
    SaveToFile();
    
    return Protocol::ErrorCode::None;
}

void ServerManager::SetServerNetworkInfo(uint64_t serverId, const std::string& ipAddress, uint16_t port) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it != serversById.end()) {
        it->second.hostIpAddress = ipAddress;
        it->second.hostPort = port;
        SaveToFile();
        printf("[SERVER] Network info set for '%s': %s:%d\n", 
               it->second.serverName.c_str(), ipAddress.c_str(), port);
    }
}

void ServerManager::SetServerOnlineStatus(uint64_t serverId, bool isOnline) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it != serversById.end()) {
        it->second.isOnline = isOnline;
        SaveToFile();
        printf("[SERVER] '%s' is now %s\n", 
               it->second.serverName.c_str(), isOnline ? "ONLINE" : "OFFLINE");
    }
}

Protocol::ErrorCode ServerManager::RenameServer(
    uint64_t serverId,
    const std::string& newName,
    uint64_t requesterId
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return Protocol::ErrorCode::ServerNotFound;
    }
    
    // Only owner can rename
    if (it->second.ownerId != requesterId) {
        return Protocol::ErrorCode::NotServerOwner;
    }
    
    // Validate new name
    if (!Models::ChatServer::IsValidServerName(newName)) {
        return Protocol::ErrorCode::InvalidServerName;
    }
    
    std::string oldName = it->second.serverName;
    it->second.serverName = newName;
    
    SaveToFile();
    
    printf("[SERVER] Renamed server '%s' to '%s'\n", oldName.c_str(), newName.c_str());
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode ServerManager::JoinServer(uint64_t serverId, uint64_t userId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return Protocol::ErrorCode::ServerNotFound;
    }
    
    Models::ChatServer& server = it->second;
    
    // Check if already a member
    if (server.IsMember(userId)) {
        // Not an error, just no-op
        return Protocol::ErrorCode::None;
    }
    
    // Check user's server limit
    std::vector<uint64_t> userServers = userDatabase.GetUserServers(userId);
    if (userServers.size() >= Models::MAX_SERVERS_PER_USER) {
        return Protocol::ErrorCode::TooManyServers;
    }
    
    // Add user to server
    server.memberIds.push_back(userId);
    
    // Add server to user's list
    userDatabase.AddUserToServer(userId, serverId);
    
    SaveToFile();
    
    printf("[SERVER] User %llu joined server '%s'\n", userId, server.serverName.c_str());
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode ServerManager::LeaveServer(uint64_t serverId, uint64_t userId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return Protocol::ErrorCode::ServerNotFound;
    }
    
    Models::ChatServer& server = it->second;
    
    // Check if member
    if (!server.IsMember(userId)) {
        return Protocol::ErrorCode::NotServerMember;
    }
    
    // Remove from member list
    server.memberIds.erase(
        std::remove(server.memberIds.begin(), server.memberIds.end(), userId),
        server.memberIds.end()
    );
    
    // Remove from user's server list
    userDatabase.RemoveUserFromServer(userId, serverId);
    
    printf("[SERVER] User %llu left server '%s'\n", userId, server.serverName.c_str());
    
    // Handle ownership transfer if owner left
    if (server.ownerId == userId) {
        if (server.memberIds.empty()) {
            // No members left, delete server
            printf("[SERVER] Last member left, deleting server '%s'\n", 
                   server.serverName.c_str());
            
            // Remove all channels
            for (uint64_t channelId : server.channelIds) {
                channelsById.erase(channelId);
            }
            
            serversById.erase(it);
        } else {
            // Transfer ownership to oldest member (first in list)
            uint64_t newOwner = server.memberIds.front();
            server.ownerId = newOwner;
            printf("[SERVER] Ownership transferred to user %llu\n", newOwner);
        }
    }
    
    SaveToFile();
    
    return Protocol::ErrorCode::None;
}

bool ServerManager::GetServer(uint64_t serverId, Models::ChatServer& outServer) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return false;
    }
    
    outServer = it->second;
    return true;
}

std::vector<Models::ChatServer> ServerManager::GetUserServers(uint64_t userId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    std::vector<Models::ChatServer> result;
    
    for (const auto& pair : serversById) {
        if (pair.second.IsMember(userId)) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<uint64_t> ServerManager::GetServerMembers(uint64_t serverId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return {};
    }
    
    return it->second.memberIds;
}

std::vector<Models::ChatServer> ServerManager::SearchServers(
    const std::string& searchTerm,
    size_t maxResults
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    std::vector<Models::ChatServer> results;
    std::string lowerSearch = searchTerm;
    std::transform(lowerSearch.begin(), lowerSearch.end(), 
                   lowerSearch.begin(), ::tolower);
    
    for (const auto& pair : serversById) {
        if (results.size() >= maxResults) break;
        
        std::string lowerName = pair.second.serverName;
        std::transform(lowerName.begin(), lowerName.end(), 
                       lowerName.begin(), ::tolower);
        
        if (lowerName.find(lowerSearch) != std::string::npos) {
            results.push_back(pair.second);
        }
    }
    
    return results;
}

//=============================================================================
// CHANNEL OPERATIONS
//=============================================================================

Protocol::ErrorCode ServerManager::CreateChannel(
    uint64_t serverId,
    const std::string& channelName,
    uint64_t requesterId,
    Models::Channel& outChannel
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return Protocol::ErrorCode::ServerNotFound;
    }
    
    Models::ChatServer& server = it->second;
    
    // Only owner can create channels
    if (server.ownerId != requesterId) {
        return Protocol::ErrorCode::NotServerOwner;
    }
    
    // Validate channel name
    if (!Models::Channel::IsValidChannelName(channelName)) {
        return Protocol::ErrorCode::InvalidChannelName;
    }
    
    // Check channel limit
    if (server.channelIds.size() >= Models::MAX_CHANNELS_PER_SERVER) {
        return Protocol::ErrorCode::TooManyChannels;
    }
    
    // Check for duplicate name in this server
    for (uint64_t channelId : server.channelIds) {
        auto channelIt = channelsById.find(channelId);
        if (channelIt != channelsById.end() && 
            channelIt->second.channelName == channelName) {
            return Protocol::ErrorCode::InvalidChannelName;  // Duplicate name
        }
    }
    
    // Create channel
    uint64_t channelId = Models::GenerateUniqueId();
    Models::Channel channel(channelId, serverId, channelName);
    
    // Store
    channelsById[channelId] = channel;
    server.channelIds.push_back(channelId);
    
    outChannel = channel;
    
    SaveToFile();
    
    printf("[CHANNEL] Created channel '#%s' in server '%s'\n", 
           channelName.c_str(), server.serverName.c_str());
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode ServerManager::DeleteChannel(uint64_t channelId, uint64_t requesterId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto channelIt = channelsById.find(channelId);
    if (channelIt == channelsById.end()) {
        return Protocol::ErrorCode::ChannelNotFound;
    }
    
    Models::Channel& channel = channelIt->second;
    
    auto serverIt = serversById.find(channel.serverId);
    if (serverIt == serversById.end()) {
        return Protocol::ErrorCode::ServerNotFound;
    }
    
    Models::ChatServer& server = serverIt->second;
    
    // Only owner can delete channels
    if (server.ownerId != requesterId) {
        return Protocol::ErrorCode::NotServerOwner;
    }
    
    // Don't allow deleting the last channel
    if (server.channelIds.size() <= 1) {
        return Protocol::ErrorCode::NotAuthorized;  // Must have at least one channel
    }
    
    // Remove from server's channel list
    server.channelIds.erase(
        std::remove(server.channelIds.begin(), server.channelIds.end(), channelId),
        server.channelIds.end()
    );
    
    printf("[CHANNEL] Deleted channel '#%s' from server '%s'\n", 
           channel.channelName.c_str(), server.serverName.c_str());
    
    // Delete channel
    channelsById.erase(channelIt);
    
    SaveToFile();
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode ServerManager::RenameChannel(
    uint64_t channelId,
    const std::string& newName,
    uint64_t requesterId
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto channelIt = channelsById.find(channelId);
    if (channelIt == channelsById.end()) {
        return Protocol::ErrorCode::ChannelNotFound;
    }
    
    Models::Channel& channel = channelIt->second;
    
    auto serverIt = serversById.find(channel.serverId);
    if (serverIt == serversById.end()) {
        return Protocol::ErrorCode::ServerNotFound;
    }
    
    // Only owner can rename channels
    if (serverIt->second.ownerId != requesterId) {
        return Protocol::ErrorCode::NotServerOwner;
    }
    
    // Validate new name
    if (!Models::Channel::IsValidChannelName(newName)) {
        return Protocol::ErrorCode::InvalidChannelName;
    }
    
    std::string oldName = channel.channelName;
    channel.channelName = newName;
    
    SaveToFile();
    
    printf("[CHANNEL] Renamed channel '#%s' to '#%s'\n", oldName.c_str(), newName.c_str());
    
    return Protocol::ErrorCode::None;
}

bool ServerManager::GetChannel(uint64_t channelId, Models::Channel& outChannel) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = channelsById.find(channelId);
    if (it == channelsById.end()) {
        return false;
    }
    
    outChannel = it->second;
    return true;
}

std::vector<Models::Channel> ServerManager::GetServerChannels(uint64_t serverId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    std::vector<Models::Channel> result;
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return result;
    }
    
    for (uint64_t channelId : it->second.channelIds) {
        auto channelIt = channelsById.find(channelId);
        if (channelIt != channelsById.end()) {
            result.push_back(channelIt->second);
        }
    }
    
    return result;
}

bool ServerManager::GetDefaultChannel(uint64_t serverId, Models::Channel& outChannel) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end() || it->second.channelIds.empty()) {
        return false;
    }
    
    uint64_t defaultChannelId = it->second.channelIds.front();
    auto channelIt = channelsById.find(defaultChannelId);
    if (channelIt == channelsById.end()) {
        return false;
    }
    
    outChannel = channelIt->second;
    return true;
}

//=============================================================================
// PERMISSION CHECKS
//=============================================================================

bool ServerManager::IsServerOwner(uint64_t serverId, uint64_t userId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return false;
    }
    
    return it->second.ownerId == userId;
}

bool ServerManager::IsServerMember(uint64_t serverId, uint64_t userId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = serversById.find(serverId);
    if (it == serversById.end()) {
        return false;
    }
    
    return it->second.IsMember(userId);
}

bool ServerManager::CanAccessChannel(uint64_t channelId, uint64_t userId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto channelIt = channelsById.find(channelId);
    if (channelIt == channelsById.end()) {
        return false;
    }
    
    auto serverIt = serversById.find(channelIt->second.serverId);
    if (serverIt == serversById.end()) {
        return false;
    }
    
    return serverIt->second.IsMember(userId);
}

bool ServerManager::GetServerByChannel(uint64_t channelId, Models::ChatServer& outServer) {
    auto channelIt = channelsById.find(channelId);
    if (channelIt == channelsById.end()) {
        return false;
    }
    
    auto serverIt = serversById.find(channelIt->second.serverId);
    if (serverIt == serversById.end()) {
        return false;
    }
    
    outServer = serverIt->second;
    return true;
}

//=============================================================================
// PERSISTENCE
//=============================================================================

bool ServerManager::SaveToFile() {
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("ServerDatabase");
    
    // Save servers
    pugi::xml_node serversNode = root.append_child("Servers");
    for (const auto& pair : serversById) {
        const Models::ChatServer& server = pair.second;
        pugi::xml_node serverNode = serversNode.append_child("Server");
        
        serverNode.append_attribute("id") = server.serverId;
        serverNode.append_attribute("name") = server.serverName.c_str();
        serverNode.append_attribute("ownerId") = server.ownerId;
        serverNode.append_attribute("createdAt") = static_cast<long long>(server.createdAt);
        
        // Save member list
        pugi::xml_node membersNode = serverNode.append_child("Members");
        for (uint64_t memberId : server.memberIds) {
            pugi::xml_node memberNode = membersNode.append_child("Member");
            memberNode.append_attribute("id") = memberId;
        }
        
        // Save channel list
        pugi::xml_node channelsNode = serverNode.append_child("Channels");
        for (uint64_t channelId : server.channelIds) {
            pugi::xml_node channelNode = channelsNode.append_child("ChannelRef");
            channelNode.append_attribute("id") = channelId;
        }
    }
    
    // Save channels (full data)
    pugi::xml_node channelsNode = root.append_child("Channels");
    for (const auto& pair : channelsById) {
        const Models::Channel& channel = pair.second;
        pugi::xml_node channelNode = channelsNode.append_child("Channel");
        
        channelNode.append_attribute("id") = channel.channelId;
        channelNode.append_attribute("serverId") = channel.serverId;
        channelNode.append_attribute("name") = channel.channelName.c_str();
        channelNode.append_attribute("createdAt") = static_cast<long long>(channel.createdAt);
    }
    
    return doc.save_file(databaseFilePath.c_str());
}

bool ServerManager::LoadFromFile() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(databaseFilePath.c_str());
    
    if (!result) {
        printf("[DB] No existing server database found, starting fresh\n");
        return false;
    }
    
    pugi::xml_node root = doc.child("ServerDatabase");
    if (!root) return false;
    
    // Load channels first (servers reference them)
    pugi::xml_node channelsNode = root.child("Channels");
    for (pugi::xml_node channelNode : channelsNode.children("Channel")) {
        Models::Channel channel;
        channel.channelId = channelNode.attribute("id").as_ullong();
        channel.serverId = channelNode.attribute("serverId").as_ullong();
        channel.channelName = channelNode.attribute("name").as_string();
        channel.createdAt = channelNode.attribute("createdAt").as_llong();
        
        channelsById[channel.channelId] = channel;
    }
    
    // Load servers
    pugi::xml_node serversNode = root.child("Servers");
    for (pugi::xml_node serverNode : serversNode.children("Server")) {
        Models::ChatServer server;
        server.serverId = serverNode.attribute("id").as_ullong();
        server.serverName = serverNode.attribute("name").as_string();
        server.ownerId = serverNode.attribute("ownerId").as_ullong();
        server.createdAt = serverNode.attribute("createdAt").as_llong();
        
        // Load members
        pugi::xml_node membersNode = serverNode.child("Members");
        for (pugi::xml_node memberNode : membersNode.children("Member")) {
            server.memberIds.push_back(memberNode.attribute("id").as_ullong());
        }
        
        // Load channel references
        pugi::xml_node channelRefsNode = serverNode.child("Channels");
        for (pugi::xml_node channelRefNode : channelRefsNode.children("ChannelRef")) {
            server.channelIds.push_back(channelRefNode.attribute("id").as_ullong());
        }
        
        serversById[server.serverId] = server;
    }
    
    printf("[DB] Loaded %zu servers and %zu channels\n", 
           serversById.size(), channelsById.size());
    
    return true;
}
