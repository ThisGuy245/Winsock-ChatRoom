/**
 * @file InviteToken.h
 * @brief Cryptographically signed invite tokens for secure access control
 * 
 * SECURITY ARCHITECTURE:
 * ----------------------
 * Invite tokens replace passwords for server access:
 * - Each token is signed by the server's private key
 * - Tokens contain expiration time, usage limits, and creator info
 * - Tokens can be revoked without affecting other users
 * - All token usage is auditable
 * 
 * THREAT MODEL:
 * - Token forgery: Prevented by ECDSA signature verification
 * - Token reuse: Limited by max_uses counter
 * - Stale tokens: Automatic expiration
 * - Token sharing: Single-use tokens prevent uncontrolled sharing
 * - Abuse tracking: created_by field enables accountability
 * 
 * WHY NOT PASSWORDS:
 * - Passwords can be shared with no accountability
 * - Passwords can't be revoked per-user
 * - Passwords don't expire automatically
 * - Passwords can be brute-forced
 * 
 * TOKEN LIFECYCLE:
 * 1. Owner creates token via server
 * 2. Server signs token with private key
 * 3. Owner shares token out-of-band (file, link, QR)
 * 4. New user presents token to join
 * 5. Server verifies signature, expiry, usage
 * 6. Token usage is logged for audit
 * 7. Owner can revoke token at any time
 */

#ifndef INVITE_TOKEN_H
#define INVITE_TOKEN_H

#include "ServerIdentity.h"
#include <string>
#include <vector>
#include <array>
#include <map>
#include <cstdint>
#include <ctime>
#include <optional>

namespace Security {

// Token sizes
constexpr size_t TOKEN_ID_SIZE = 16;        // 128-bit unique ID

/**
 * @brief Permission flags that can be granted via invite
 */
enum class InvitePermission : uint64_t {
    None = 0,
    SendMessages = 1 << 0,      // Can send chat messages
    ReadMessages = 1 << 1,      // Can read chat messages
    CreateChannels = 1 << 2,    // Can create new channels
    ManageChannels = 1 << 3,    // Can edit/delete channels
    InviteOthers = 1 << 4,      // Can create new invites
    KickMembers = 1 << 5,       // Can kick other members
    BanMembers = 1 << 6,        // Can ban other members
    ManageServer = 1 << 7,      // Full server management
    
    // Common permission sets
    Member = SendMessages | ReadMessages,
    Moderator = Member | KickMembers | InviteOthers,
    Admin = Moderator | CreateChannels | ManageChannels | BanMembers,
    Owner = 0xFFFFFFFFFFFFFFFF  // All permissions
};

// Bitwise operators for permissions
inline InvitePermission operator|(InvitePermission a, InvitePermission b) {
    return static_cast<InvitePermission>(
        static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline InvitePermission operator&(InvitePermission a, InvitePermission b) {
    return static_cast<InvitePermission>(
        static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline bool HasPermission(InvitePermission granted, InvitePermission required) {
    return (static_cast<uint64_t>(granted) & static_cast<uint64_t>(required)) != 0;
}

/**
 * @brief Status of an invite token
 */
enum class TokenStatus {
    Valid,              // Token can be used
    Expired,            // Past expiration time
    Exhausted,          // max_uses reached
    Revoked,            // Manually revoked by owner
    InvalidSignature,   // Signature verification failed
    WrongServer,        // Token is for a different server
    Malformed           // Token data is corrupted
};

const char* TokenStatusToString(TokenStatus status);

/**
 * @brief Represents a cryptographically signed invite token
 * 
 * TOKEN STRUCTURE:
 * ┌─────────────────────────────────────────┐
 * │ token_id:      16 bytes (unique ID)     │
 * │ server_id:     32 bytes (bound server)  │
 * │ created_by:    8 bytes (creator user)   │
 * │ created_at:    8 bytes (timestamp)      │
 * │ expires_at:    8 bytes (expiration)     │
 * │ max_uses:      4 bytes (usage limit)    │
 * │ permissions:   8 bytes (access flags)   │
 * │ ─────────────────────────────────────── │
 * │ signature:     64 bytes (ECDSA sig)     │
 * └─────────────────────────────────────────┘
 */
class InviteToken {
public:
    /**
     * @brief Create a new invite token
     * @param serverIdentity Server's identity (needs private key for signing)
     * @param createdBy User ID of the creator
     * @param expiresInSeconds How long until token expires (0 = never)
     * @param maxUses Maximum number of times token can be used (0 = unlimited)
     * @param permissions Permissions granted by this invite
     * @return Signed token or nullopt on failure
     */
    static std::optional<InviteToken> Create(
        const ServerIdentity& serverIdentity,
        uint64_t createdBy,
        uint32_t expiresInSeconds = 86400,  // Default: 24 hours
        uint32_t maxUses = 1,               // Default: single-use
        InvitePermission permissions = InvitePermission::Member
    );
    
    /**
     * @brief Parse a token from its serialized form
     * @param data Serialized token bytes
     * @return Parsed token (not yet validated)
     */
    static std::optional<InviteToken> Parse(const std::vector<uint8_t>& data);
    
    /**
     * @brief Parse a token from base64 string
     * @param encoded Base64-encoded token
     * @return Parsed token (not yet validated)
     */
    static std::optional<InviteToken> FromBase64(const std::string& encoded);
    
    /**
     * @brief Validate token signature and check constraints
     * @param serverIdentity Server to validate against
     * @return TokenStatus indicating result
     * 
     * SECURITY: This verifies:
     * 1. Signature is valid for this server's public key
     * 2. Server ID matches expected server
     * 3. Token has not expired
     * 4. Token has uses remaining (if max_uses > 0)
     * 
     * NOTE: Does NOT check revocation (server must track separately)
     */
    TokenStatus Validate(const ServerIdentity& serverIdentity) const;
    
    /**
     * @brief Serialize token to bytes
     */
    std::vector<uint8_t> Serialize() const;
    
    /**
     * @brief Encode token as base64 string for easy sharing
     */
    std::string ToBase64() const;
    
    /**
     * @brief Generate a shareable invite link
     * @param baseUrl Application URL scheme (e.g., "chatapp://")
     */
    std::string ToInviteLink(const std::string& baseUrl = "chatapp://") const;
    
    // Getters
    const std::array<uint8_t, TOKEN_ID_SIZE>& GetTokenId() const { return tokenId; }
    const std::array<uint8_t, SERVER_ID_SIZE>& GetServerId() const { return serverId; }
    uint64_t GetCreatedBy() const { return createdBy; }
    std::time_t GetCreatedAt() const { return createdAt; }
    std::time_t GetExpiresAt() const { return expiresAt; }
    uint32_t GetMaxUses() const { return maxUses; }
    InvitePermission GetPermissions() const { return permissions; }
    
    /**
     * @brief Get Token ID as hex string
     */
    std::string GetTokenIdHex() const;
    
    /**
     * @brief Check if token has expired
     */
    bool IsExpired() const;
    
    /**
     * @brief Check if token is for a specific server
     */
    bool IsForServer(const ServerIdentity& server) const;
    bool IsForServer(const std::array<uint8_t, SERVER_ID_SIZE>& serverId) const;

private:
    InviteToken() = default;
    
    // Token data (signed portion)
    std::array<uint8_t, TOKEN_ID_SIZE> tokenId;     // Unique identifier
    std::array<uint8_t, SERVER_ID_SIZE> serverId;   // Which server this is for
    uint64_t createdBy;                              // User who created it
    std::time_t createdAt;                           // Creation timestamp
    std::time_t expiresAt;                           // Expiration timestamp (0 = never)
    uint32_t maxUses;                                // Max uses (0 = unlimited)
    InvitePermission permissions;                    // Granted permissions
    
    // Signature (not signed, obviously)
    std::array<uint8_t, SIGNATURE_SIZE> signature;
    
    /**
     * @brief Get the data that should be signed
     */
    std::vector<uint8_t> GetSignableData() const;
};

/**
 * @brief Manages invite tokens for a server
 * 
 * Tracks:
 * - Active tokens
 * - Usage counts
 * - Revoked tokens
 * - Audit log
 */
class InviteManager {
public:
    explicit InviteManager(const std::string& dataFilePath);
    ~InviteManager();
    
    /**
     * @brief Create a new invite token
     * @param serverIdentity Server's identity
     * @param createdBy User ID creating the invite
     * @param expiresInSeconds Expiration time
     * @param maxUses Usage limit
     * @param permissions Permissions to grant
     * @return The created token or nullopt on failure
     */
    std::optional<InviteToken> CreateInvite(
        const ServerIdentity& serverIdentity,
        uint64_t createdBy,
        uint32_t expiresInSeconds = 86400,
        uint32_t maxUses = 1,
        InvitePermission permissions = InvitePermission::Member
    );
    
    /**
     * @brief Validate and consume an invite token
     * @param token Token to validate
     * @param serverIdentity Server's identity
     * @param newUserId User ID of the person using the token
     * @return TokenStatus indicating result
     * 
     * If valid, decrements usage count and logs the use
     */
    TokenStatus ValidateAndConsume(
        const InviteToken& token,
        const ServerIdentity& serverIdentity,
        uint64_t newUserId
    );
    
    /**
     * @brief Revoke an invite token
     * @param tokenId Token to revoke
     * @param revokedBy User ID doing the revocation
     * @return true if token was found and revoked
     */
    bool RevokeInvite(const std::array<uint8_t, TOKEN_ID_SIZE>& tokenId,
                      uint64_t revokedBy);
    
    /**
     * @brief Check if a token is revoked
     */
    bool IsRevoked(const std::array<uint8_t, TOKEN_ID_SIZE>& tokenId) const;
    
    /**
     * @brief Get remaining uses for a token
     * @return -1 if token not found, 0 if exhausted, >0 for remaining uses
     */
    int GetRemainingUses(const std::array<uint8_t, TOKEN_ID_SIZE>& tokenId) const;
    
    /**
     * @brief Save state to file
     */
    void SaveToFile();
    
    /**
     * @brief Load state from file
     */
    void LoadFromFile();

private:
    std::string dataFilePath;
    
    // Token usage tracking: tokenId -> remaining uses
    std::map<std::string, uint32_t> tokenUsage;
    
    // Revoked tokens: tokenId hex -> revocation time
    std::map<std::string, std::time_t> revokedTokens;
    
    // Audit log entry
    struct AuditEntry {
        std::time_t timestamp;
        std::string tokenIdHex;
        uint64_t userId;
        std::string action;  // "created", "used", "revoked"
    };
    std::vector<AuditEntry> auditLog;
    
    void LogAudit(const std::string& tokenIdHex, uint64_t userId, const std::string& action);
};

/**
 * @brief Base64 encoding utilities
 */
std::string Base64Encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> Base64Decode(const std::string& encoded);

} // namespace Security

#endif // INVITE_TOKEN_H
