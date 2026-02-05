/**
 * @file UserDatabase.cpp
 * @brief Implementation of secure user storage and authentication
 * 
 * SECURITY NOTE:
 * This implementation uses SHA-256 for password hashing, which is NOT
 * ideal for passwords (no work factor). For a production system, use:
 * - bcrypt (recommended)
 * - Argon2 (newer, memory-hard)
 * - PBKDF2 (widely available)
 * 
 * SHA-256 is used here to avoid external dependencies while still
 * demonstrating proper security patterns (salt, constant-time comparison).
 */

#include "UserDatabase.h"
#include "NetProtocol.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>

// Windows crypto API for secure random and hashing
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")

//=============================================================================
// CONSTRUCTOR / DESTRUCTOR
//=============================================================================

UserDatabase::UserDatabase(const std::string& databasePath)
    : databaseFilePath(databasePath) {
    LoadFromFile();
}

UserDatabase::~UserDatabase() {
    SaveToFile();
    
    // Securely clear all password data from memory
    for (auto& pair : passwordsByUserId) {
        SecureClearString(pair.second.salt);
        SecureClearString(pair.second.hash);
    }
}

//=============================================================================
// USER REGISTRATION & AUTHENTICATION
//=============================================================================

Protocol::ErrorCode UserDatabase::RegisterUser(
    const std::string& username,
    const std::string& password,
    uint64_t& outUserId
) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    // Validate username format
    if (!Models::User::IsValidUsername(username)) {
        return Protocol::ErrorCode::InvalidUsername;
    }
    
    // Check if username already exists (case-insensitive)
    std::string lowerUsername = username;
    std::transform(lowerUsername.begin(), lowerUsername.end(), 
                   lowerUsername.begin(), ::tolower);
    
    for (const auto& pair : userIdByUsername) {
        std::string existingLower = pair.first;
        std::transform(existingLower.begin(), existingLower.end(),
                       existingLower.begin(), ::tolower);
        if (existingLower == lowerUsername) {
            return Protocol::ErrorCode::UsernameAlreadyExists;
        }
    }
    
    // Validate password (minimum requirements)
    if (password.length() < 8) {
        return Protocol::ErrorCode::InvalidPassword;
    }
    
    // Generate unique user ID
    uint64_t userId = Models::GenerateUniqueId();
    
    // Generate salt and hash password
    std::string salt = GenerateSalt();
    std::string passwordHash = HashPassword(password, salt);
    
    // Create user
    Models::User newUser(userId, username);
    
    // Store user
    usersById[userId] = newUser;
    userIdByUsername[username] = userId;
    
    // Store password data separately
    PasswordData passwordData;
    passwordData.salt = salt;
    passwordData.hash = passwordHash;
    passwordsByUserId[userId] = passwordData;
    
    outUserId = userId;
    
    // Persist to disk
    SaveToFile();
    
    printf("[AUTH] User registered: %s (ID: %llu)\n", username.c_str(), userId);
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode UserDatabase::AuthenticateUser(
    const std::string& username,
    const std::string& password,
    Models::Session& outSession
) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    // Find user by username
    auto usernameIt = userIdByUsername.find(username);
    if (usernameIt == userIdByUsername.end()) {
        // Don't reveal whether username exists
        return Protocol::ErrorCode::InvalidCredentials;
    }
    
    uint64_t userId = usernameIt->second;
    
    // Get password data
    auto passwordIt = passwordsByUserId.find(userId);
    if (passwordIt == passwordsByUserId.end()) {
        // Should never happen, but handle gracefully
        return Protocol::ErrorCode::InternalError;
    }
    
    const PasswordData& storedPassword = passwordIt->second;
    
    // Hash the provided password with stored salt
    std::string providedHash = HashPassword(password, storedPassword.salt);
    
    // SECURITY: Constant-time comparison to prevent timing attacks
    if (!ConstantTimeCompare(providedHash, storedPassword.hash)) {
        printf("[AUTH] Failed login attempt for user: %s\n", username.c_str());
        return Protocol::ErrorCode::InvalidCredentials;
    }
    
    // Authentication successful - create session
    std::string sessionToken = GenerateSessionToken();
    
    Models::Session session(userId, sessionToken);
    sessionsByToken[sessionToken] = session;
    
    // Update user's last login time
    auto userIt = usersById.find(userId);
    if (userIt != usersById.end()) {
        userIt->second.lastLoginAt = std::time(nullptr);
        userIt->second.isOnline = true;
    }
    
    outSession = session;
    
    printf("[AUTH] User logged in: %s (Session: %s...)\n", 
           username.c_str(), sessionToken.substr(0, 8).c_str());
    
    return Protocol::ErrorCode::None;
}

bool UserDatabase::ValidateSession(const std::string& sessionToken, uint64_t& outUserId) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = sessionsByToken.find(sessionToken);
    if (it == sessionsByToken.end()) {
        return false;
    }
    
    Models::Session& session = it->second;
    
    // Check if expired
    if (session.IsExpired()) {
        sessionsByToken.erase(it);
        return false;
    }
    
    outUserId = session.userId;
    return true;
}

void UserDatabase::InvalidateSession(const std::string& sessionToken) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = sessionsByToken.find(sessionToken);
    if (it != sessionsByToken.end()) {
        uint64_t userId = it->second.userId;
        sessionsByToken.erase(it);
        
        // Check if user has any other active sessions
        bool hasOtherSessions = false;
        for (const auto& pair : sessionsByToken) {
            if (pair.second.userId == userId) {
                hasOtherSessions = true;
                break;
            }
        }
        
        // If no other sessions, mark user as offline
        if (!hasOtherSessions) {
            auto userIt = usersById.find(userId);
            if (userIt != usersById.end()) {
                userIt->second.isOnline = false;
            }
        }
        
        printf("[AUTH] Session invalidated\n");
    }
}

void UserDatabase::UpdateSessionActivity(const std::string& sessionToken) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = sessionsByToken.find(sessionToken);
    if (it != sessionsByToken.end()) {
        it->second.UpdateActivity();
    }
}

//=============================================================================
// USER QUERIES
//=============================================================================

bool UserDatabase::GetUserById(uint64_t userId, Models::User& outUser) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = usersById.find(userId);
    if (it == usersById.end()) {
        return false;
    }
    
    outUser = it->second;
    return true;
}

bool UserDatabase::GetUserByUsername(const std::string& username, Models::User& outUser) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto usernameIt = userIdByUsername.find(username);
    if (usernameIt == userIdByUsername.end()) {
        return false;
    }
    
    auto userIt = usersById.find(usernameIt->second);
    if (userIt == usersById.end()) {
        return false;
    }
    
    outUser = userIt->second;
    return true;
}

bool UserDatabase::UsernameExists(const std::string& username) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    return userIdByUsername.find(username) != userIdByUsername.end();
}

std::vector<Models::User> UserDatabase::SearchUsers(const std::string& prefix, size_t maxResults) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    std::vector<Models::User> results;
    std::string lowerPrefix = prefix;
    std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);
    
    for (const auto& pair : usersById) {
        if (results.size() >= maxResults) break;
        
        std::string lowerUsername = pair.second.username;
        std::transform(lowerUsername.begin(), lowerUsername.end(), 
                       lowerUsername.begin(), ::tolower);
        
        if (lowerUsername.find(lowerPrefix) == 0) {
            results.push_back(pair.second);
        }
    }
    
    return results;
}

//=============================================================================
// USER UPDATES
//=============================================================================

void UserDatabase::SetUserOnlineStatus(uint64_t userId, bool isOnline) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = usersById.find(userId);
    if (it != usersById.end()) {
        it->second.isOnline = isOnline;
    }
}

void UserDatabase::AddUserToServer(uint64_t userId, uint64_t serverId) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = usersById.find(userId);
    if (it != usersById.end()) {
        auto& servers = it->second.serverIds;
        if (std::find(servers.begin(), servers.end(), serverId) == servers.end()) {
            servers.push_back(serverId);
        }
    }
}

void UserDatabase::RemoveUserFromServer(uint64_t userId, uint64_t serverId) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = usersById.find(userId);
    if (it != usersById.end()) {
        auto& servers = it->second.serverIds;
        servers.erase(
            std::remove(servers.begin(), servers.end(), serverId),
            servers.end()
        );
    }
}

std::vector<uint64_t> UserDatabase::GetUserServers(uint64_t userId) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = usersById.find(userId);
    if (it != usersById.end()) {
        return it->second.serverIds;
    }
    return {};
}

//=============================================================================
// FRIEND MANAGEMENT
//=============================================================================

void UserDatabase::AddFriendship(uint64_t userId1, uint64_t userId2) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    // Add bidirectional friendship
    auto it1 = usersById.find(userId1);
    auto it2 = usersById.find(userId2);
    
    if (it1 != usersById.end() && it2 != usersById.end()) {
        auto& friends1 = it1->second.friendIds;
        auto& friends2 = it2->second.friendIds;
        
        if (std::find(friends1.begin(), friends1.end(), userId2) == friends1.end()) {
            friends1.push_back(userId2);
        }
        if (std::find(friends2.begin(), friends2.end(), userId1) == friends2.end()) {
            friends2.push_back(userId1);
        }
        
        SaveToFile();
    }
}

void UserDatabase::RemoveFriendship(uint64_t userId1, uint64_t userId2) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it1 = usersById.find(userId1);
    auto it2 = usersById.find(userId2);
    
    if (it1 != usersById.end()) {
        auto& friends = it1->second.friendIds;
        friends.erase(std::remove(friends.begin(), friends.end(), userId2), friends.end());
    }
    
    if (it2 != usersById.end()) {
        auto& friends = it2->second.friendIds;
        friends.erase(std::remove(friends.begin(), friends.end(), userId1), friends.end());
    }
    
    SaveToFile();
}

bool UserDatabase::AreFriends(uint64_t userId1, uint64_t userId2) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    auto it = usersById.find(userId1);
    if (it == usersById.end()) return false;
    
    const auto& friends = it->second.friendIds;
    return std::find(friends.begin(), friends.end(), userId2) != friends.end();
}

std::vector<Models::User> UserDatabase::GetFriends(uint64_t userId) {
    std::lock_guard<std::mutex> lock(databaseMutex);
    
    std::vector<Models::User> friends;
    
    auto it = usersById.find(userId);
    if (it != usersById.end()) {
        for (uint64_t friendId : it->second.friendIds) {
            auto friendIt = usersById.find(friendId);
            if (friendIt != usersById.end()) {
                friends.push_back(friendIt->second);
            }
        }
    }
    
    return friends;
}

//=============================================================================
// CRYPTOGRAPHIC HELPERS
//=============================================================================

std::string UserDatabase::GenerateSecureRandom(size_t length) {
    std::vector<BYTE> buffer(length);
    
    HCRYPTPROV hProvider = 0;
    if (!CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_AES, 
                               CRYPT_VERIFYCONTEXT)) {
        // Fallback to less secure random if crypto provider unavailable
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < length; i++) {
            buffer[i] = static_cast<BYTE>(dis(gen));
        }
    } else {
        CryptGenRandom(hProvider, static_cast<DWORD>(length), buffer.data());
        CryptReleaseContext(hProvider, 0);
    }
    
    // Convert to hex string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (BYTE b : buffer) {
        ss << std::setw(2) << static_cast<int>(b);
    }
    
    return ss.str();
}

std::string UserDatabase::GenerateSalt() {
    return GenerateSecureRandom(32);  // 256-bit salt
}

std::string UserDatabase::GenerateSessionToken() {
    return GenerateSecureRandom(32);  // 256-bit token
}

std::string UserDatabase::HashPassword(const std::string& password, const std::string& salt) {
    // Combine salt and password
    std::string combined = salt + password;
    
    HCRYPTPROV hProvider = 0;
    HCRYPTHASH hHash = 0;
    std::string result;
    
    if (!CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_AES, 
                               CRYPT_VERIFYCONTEXT)) {
        return "";
    }
    
    if (!CryptCreateHash(hProvider, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProvider, 0);
        return "";
    }
    
    if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(combined.c_str()),
                       static_cast<DWORD>(combined.length()), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProvider, 0);
        return "";
    }
    
    DWORD hashLen = 32;  // SHA-256 = 32 bytes
    std::vector<BYTE> hashBuffer(hashLen);
    
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hashBuffer.data(), &hashLen, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProvider, 0);
        return "";
    }
    
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProvider, 0);
    
    // Convert to hex string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (BYTE b : hashBuffer) {
        ss << std::setw(2) << static_cast<int>(b);
    }
    
    // SECURITY: Clear the combined string from memory
    SecureClearString(combined);
    
    return ss.str();
}

bool UserDatabase::ConstantTimeCompare(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) {
        return false;
    }
    
    volatile unsigned char result = 0;
    for (size_t i = 0; i < a.length(); i++) {
        result |= a[i] ^ b[i];
    }
    
    return result == 0;
}

void UserDatabase::SecureClearString(std::string& str) {
    if (!str.empty()) {
        volatile char* p = &str[0];
        size_t len = str.size();
        while (len--) {
            *p++ = 0;
        }
        str.clear();
    }
}

//=============================================================================
// PERSISTENCE
//=============================================================================

bool UserDatabase::SaveToFile() {
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("UserDatabase");
    
    // Save users
    pugi::xml_node usersNode = root.append_child("Users");
    for (const auto& pair : usersById) {
        const Models::User& user = pair.second;
        pugi::xml_node userNode = usersNode.append_child("User");
        
        userNode.append_attribute("id") = user.userId;
        userNode.append_attribute("username") = user.username.c_str();
        userNode.append_attribute("createdAt") = static_cast<long long>(user.createdAt);
        userNode.append_attribute("lastLoginAt") = static_cast<long long>(user.lastLoginAt);
        
        // Save password data
        auto passIt = passwordsByUserId.find(user.userId);
        if (passIt != passwordsByUserId.end()) {
            userNode.append_attribute("salt") = passIt->second.salt.c_str();
            userNode.append_attribute("hash") = passIt->second.hash.c_str();
        }
        
        // Save server memberships
        pugi::xml_node serversNode = userNode.append_child("Servers");
        for (uint64_t serverId : user.serverIds) {
            pugi::xml_node serverNode = serversNode.append_child("Server");
            serverNode.append_attribute("id") = serverId;
        }
        
        // Save friends
        pugi::xml_node friendsNode = userNode.append_child("Friends");
        for (uint64_t friendId : user.friendIds) {
            pugi::xml_node friendNode = friendsNode.append_child("Friend");
            friendNode.append_attribute("id") = friendId;
        }
    }
    
    return doc.save_file(databaseFilePath.c_str());
}

bool UserDatabase::LoadFromFile() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(databaseFilePath.c_str());
    
    if (!result) {
        printf("[DB] No existing database found, starting fresh\n");
        return false;
    }
    
    pugi::xml_node root = doc.child("UserDatabase");
    if (!root) return false;
    
    // Load users
    pugi::xml_node usersNode = root.child("Users");
    for (pugi::xml_node userNode : usersNode.children("User")) {
        Models::User user;
        user.userId = userNode.attribute("id").as_ullong();
        user.username = userNode.attribute("username").as_string();
        user.createdAt = userNode.attribute("createdAt").as_llong();
        user.lastLoginAt = userNode.attribute("lastLoginAt").as_llong();
        user.isOnline = false;  // Always start offline
        
        // Load password data
        PasswordData passwordData;
        passwordData.salt = userNode.attribute("salt").as_string();
        passwordData.hash = userNode.attribute("hash").as_string();
        
        if (!passwordData.salt.empty() && !passwordData.hash.empty()) {
            passwordsByUserId[user.userId] = passwordData;
        }
        
        // Load server memberships
        pugi::xml_node serversNode = userNode.child("Servers");
        for (pugi::xml_node serverNode : serversNode.children("Server")) {
            user.serverIds.push_back(serverNode.attribute("id").as_ullong());
        }
        
        // Load friends
        pugi::xml_node friendsNode = userNode.child("Friends");
        for (pugi::xml_node friendNode : friendsNode.children("Friend")) {
            user.friendIds.push_back(friendNode.attribute("id").as_ullong());
        }
        
        usersById[user.userId] = user;
        userIdByUsername[user.username] = user.userId;
    }
    
    printf("[DB] Loaded %zu users from database\n", usersById.size());
    return true;
}
