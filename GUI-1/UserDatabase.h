#ifndef USER_DATABASE_H
#define USER_DATABASE_H

/**
 * @file UserDatabase.h
 * @brief Secure user storage and authentication system
 * 
 * SECURITY IMPLEMENTATION:
 * 
 * 1. PASSWORD HASHING:
 *    - Passwords are hashed using SHA-256 with per-user salt
 *    - Salt is 32 random bytes, generated securely
 *    - Hash = SHA256(salt + password)
 *    - In production: Use bcrypt/Argon2 instead (requires external lib)
 * 
 * 2. CONSTANT-TIME COMPARISON:
 *    - Password verification uses constant-time comparison
 *    - Prevents timing attacks
 * 
 * 3. SECURE MEMORY:
 *    - Passwords are cleared from memory after use
 *    - Hashes are stored, never plaintext
 * 
 * 4. SESSION TOKENS:
 *    - Generated using cryptographically secure random
 *    - 256 bits of entropy
 *    - Tokens are hashed before storage (optional)
 * 
 * STORAGE:
 *    Uses XML for simplicity (pugixml already in project)
 *    In production: Use encrypted SQLite or similar
 */

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include "Models.h"
#include "Protocol.h"
#include "pugixml.hpp"

class UserDatabase {
public:
    /**
     * @brief Initialize the database with the given file path
     * @param databasePath Path to the XML database file
     */
    explicit UserDatabase(const std::string& databasePath);
    
    ~UserDatabase();
    
    // =========================================================================
    // USER REGISTRATION & AUTHENTICATION
    // =========================================================================
    
    /**
     * @brief Register a new user account
     * 
     * SECURITY:
     * - Username is validated for format and uniqueness
     * - Password is hashed with unique salt before storage
     * - Password is cleared from memory after hashing
     * 
     * @param username The desired username
     * @param password The plaintext password (will be hashed)
     * @param outUserId Output: the created user's ID
     * @return Error code (None on success)
     */
    Protocol::ErrorCode RegisterUser(
        const std::string& username,
        const std::string& password,
        uint64_t& outUserId
    );
    
    /**
     * @brief Authenticate a user and create a session
     * 
     * SECURITY:
     * - Uses constant-time password comparison
     * - Generates secure session token on success
     * - Password is cleared from memory after verification
     * 
     * @param username The username
     * @param password The plaintext password
     * @param outSession Output: the created session
     * @return Error code (None on success)
     */
    Protocol::ErrorCode AuthenticateUser(
        const std::string& username,
        const std::string& password,
        Models::Session& outSession
    );
    
    /**
     * @brief Validate a session token
     * 
     * @param sessionToken The token to validate
     * @param outUserId Output: the user ID if valid
     * @return True if session is valid and not expired
     */
    bool ValidateSession(const std::string& sessionToken, uint64_t& outUserId);
    
    /**
     * @brief Invalidate (logout) a session
     * @param sessionToken The token to invalidate
     */
    void InvalidateSession(const std::string& sessionToken);
    
    /**
     * @brief Update session activity (extends expiry)
     * @param sessionToken The session to update
     */
    void UpdateSessionActivity(const std::string& sessionToken);
    
    // =========================================================================
    // USER QUERIES
    // =========================================================================
    
    /**
     * @brief Get user by ID
     * @param userId The user ID
     * @param outUser Output: the user data
     * @return True if user found
     */
    bool GetUserById(uint64_t userId, Models::User& outUser);
    
    /**
     * @brief Get user by username
     * @param username The username
     * @param outUser Output: the user data
     * @return True if user found
     */
    bool GetUserByUsername(const std::string& username, Models::User& outUser);
    
    /**
     * @brief Check if a username exists
     * @param username The username to check
     * @return True if username is taken
     */
    bool UsernameExists(const std::string& username);
    
    /**
     * @brief Search for users by username prefix
     * @param prefix The search prefix
     * @param maxResults Maximum results to return
     * @return List of matching users
     */
    std::vector<Models::User> SearchUsers(const std::string& prefix, size_t maxResults = 20);
    
    // =========================================================================
    // USER UPDATES
    // =========================================================================
    
    /**
     * @brief Update user's online status
     * @param userId The user ID
     * @param isOnline The new online status
     */
    void SetUserOnlineStatus(uint64_t userId, bool isOnline);
    
    /**
     * @brief Add a server to user's server list
     * @param userId The user ID
     * @param serverId The server ID to add
     */
    void AddUserToServer(uint64_t userId, uint64_t serverId);
    
    /**
     * @brief Remove a server from user's server list
     * @param userId The user ID
     * @param serverId The server ID to remove
     */
    void RemoveUserFromServer(uint64_t userId, uint64_t serverId);
    
    /**
     * @brief Get list of server IDs a user belongs to
     * @param userId The user ID
     * @return Vector of server IDs
     */
    std::vector<uint64_t> GetUserServers(uint64_t userId);
    
    // =========================================================================
    // FRIEND MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Add a friend relationship (bidirectional)
     * @param userId1 First user
     * @param userId2 Second user
     */
    void AddFriendship(uint64_t userId1, uint64_t userId2);
    
    /**
     * @brief Remove a friend relationship
     * @param userId1 First user
     * @param userId2 Second user
     */
    void RemoveFriendship(uint64_t userId1, uint64_t userId2);
    
    /**
     * @brief Check if two users are friends
     * @param userId1 First user
     * @param userId2 Second user
     * @return True if they are friends
     */
    bool AreFriends(uint64_t userId1, uint64_t userId2);
    
    /**
     * @brief Get user's friend list
     * @param userId The user ID
     * @return Vector of friend user data
     */
    std::vector<Models::User> GetFriends(uint64_t userId);
    
    // =========================================================================
    // PERSISTENCE
    // =========================================================================
    
    /**
     * @brief Save all data to disk
     * @return True on success
     */
    bool SaveToFile();
    
    /**
     * @brief Load data from disk
     * @return True on success
     */
    bool LoadFromFile();
    
private:
    // Database file path
    std::string databaseFilePath;
    
    // In-memory data
    std::map<uint64_t, Models::User> usersById;
    std::map<std::string, uint64_t> userIdByUsername;
    std::map<std::string, Models::Session> sessionsByToken;
    
    // Password hashes stored separately for isolation
    struct PasswordData {
        std::string salt;        // Base64 encoded
        std::string hash;        // Base64 encoded
    };
    std::map<uint64_t, PasswordData> passwordsByUserId;
    
    // Thread safety
    mutable std::mutex databaseMutex;
    
    // =========================================================================
    // CRYPTOGRAPHIC HELPERS (Private)
    // =========================================================================
    
    /**
     * @brief Generate cryptographically secure random bytes
     * @param length Number of bytes to generate
     * @return Hex-encoded random string
     */
    static std::string GenerateSecureRandom(size_t length);
    
    /**
     * @brief Generate a secure salt for password hashing
     * @return Base64-encoded salt
     */
    static std::string GenerateSalt();
    
    /**
     * @brief Generate a secure session token
     * @return Hex-encoded token (64 characters = 256 bits)
     */
    static std::string GenerateSessionToken();
    
    /**
     * @brief Hash a password with salt
     * 
     * NOTE: Uses SHA-256 for simplicity. In production, use bcrypt/Argon2.
     * 
     * @param password The plaintext password
     * @param salt The salt to use
     * @return Hex-encoded hash
     */
    static std::string HashPassword(const std::string& password, const std::string& salt);
    
    /**
     * @brief Constant-time string comparison
     * 
     * SECURITY: Prevents timing attacks on password verification
     * 
     * @param a First string
     * @param b Second string
     * @return True if equal
     */
    static bool ConstantTimeCompare(const std::string& a, const std::string& b);
    
    /**
     * @brief Securely clear a string from memory
     * @param str The string to clear
     */
    static void SecureClearString(std::string& str);
    
    // XML helpers
    void SaveUsersToXml(pugi::xml_node& root);
    void LoadUsersFromXml(pugi::xml_node& root);
};

#endif // USER_DATABASE_H
