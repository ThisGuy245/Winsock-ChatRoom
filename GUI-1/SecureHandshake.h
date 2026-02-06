/**
 * @file SecureHandshake.h
 * @brief Secure handshake protocol for server authentication and client join
 * 
 * PROTOCOL OVERVIEW:
 * ──────────────────
 * This implements a challenge-response authentication protocol:
 * 
 *   Client                                          Server
 *      │                                               │
 *      │────────── ClientHello ───────────────────────>│
 *      │   { version, client_nonce }                   │
 *      │                                               │
 *      │<───────── ServerHello ────────────────────────│
 *      │   { server_id, public_key, server_nonce,      │
 *      │     signature(client_nonce || server_nonce) } │
 *      │                                               │
 *      │   [Client verifies server identity]           │
 *      │                                               │
 *      │────────── JoinRequest ───────────────────────>│
 *      │   { invite_token, timestamp }                 │
 *      │                                               │
 *      │   [Server validates invite]                   │
 *      │                                               │
 *      │<───────── JoinResponse ───────────────────────│
 *      │   { status, session_token, user_info }        │
 *      │                                               │
 * 
 * SECURITY PROPERTIES:
 * - Server authentication: Client verifies server's signature
 * - Replay protection: Fresh nonces, timestamps with bounded skew
 * - Man-in-the-middle prevention: Signature binds to specific server identity
 * - Authorization: Valid invite token required
 * 
 * THREAT MODEL:
 * - Network adversary can observe and modify traffic
 * - Adversary cannot forge signatures without private key
 * - Adversary cannot reuse captured messages (nonce binding)
 */

#ifndef SECURE_HANDSHAKE_H
#define SECURE_HANDSHAKE_H

#include "ServerIdentity.h"
#include "InviteToken.h"
#include <string>
#include <vector>
#include <cstdint>
#include <ctime>
#include <optional>
#include <functional>

namespace Security {

// Protocol version for compatibility checking
constexpr uint16_t PROTOCOL_VERSION = 1;

// Maximum allowed clock skew for timestamp validation (seconds)
constexpr int64_t MAX_CLOCK_SKEW = 300; // 5 minutes

/**
 * @brief Message types in the handshake protocol
 */
enum class HandshakeMessageType : uint8_t {
    ClientHello = 0x01,
    ServerHello = 0x02,
    JoinRequest = 0x03,
    JoinResponse = 0x04,
    Error = 0xFF
};

/**
 * @brief Join response status codes
 */
enum class JoinStatus : uint8_t {
    Success = 0x00,
    
    // Server-side errors
    InvalidInvite = 0x01,
    InviteExpired = 0x02,
    InviteExhausted = 0x03,
    InviteRevoked = 0x04,
    ServerFull = 0x05,
    Banned = 0x06,
    
    // Protocol errors
    VersionMismatch = 0x10,
    InvalidMessage = 0x11,
    SignatureInvalid = 0x12,
    TimestampInvalid = 0x13,
    
    // Client-side errors (for local use)
    ServerIdMismatch = 0x20,
    ConnectionFailed = 0x21,
    Timeout = 0x22
};

const char* JoinStatusToString(JoinStatus status);

//=============================================================================
// PROTOCOL MESSAGES
//=============================================================================

/**
 * @brief ClientHello - First message from client
 * 
 * Contains:
 * - Protocol version for compatibility
 * - Random nonce for replay protection
 */
struct ClientHello {
    uint16_t protocolVersion;
    std::array<uint8_t, NONCE_SIZE> clientNonce;
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<ClientHello> Parse(const std::vector<uint8_t>& data);
};

/**
 * @brief ServerHello - Server's identity proof
 * 
 * Contains:
 * - Server's cryptographic identity
 * - Challenge signature proving ownership of private key
 * 
 * SECURITY: Client MUST verify:
 * 1. server_id == SHA256(public_key)
 * 2. server_id matches expected server
 * 3. signature is valid for (client_nonce || server_nonce)
 */
struct ServerHello {
    std::array<uint8_t, SERVER_ID_SIZE> serverId;
    std::vector<uint8_t> publicKey;
    std::array<uint8_t, NONCE_SIZE> serverNonce;
    std::array<uint8_t, SIGNATURE_SIZE> signature;  // Signs client_nonce || server_nonce
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<ServerHello> Parse(const std::vector<uint8_t>& data);
    
    /**
     * @brief Create and sign a ServerHello
     */
    static std::optional<ServerHello> Create(
        const ServerIdentity& serverIdentity,
        const std::array<uint8_t, NONCE_SIZE>& clientNonce
    );
    
    /**
     * @brief Verify the ServerHello is authentic
     * @param clientNonce The nonce sent in ClientHello
     * @param expectedServerId Optional: expected server ID to match
     */
    bool Verify(const std::array<uint8_t, NONCE_SIZE>& clientNonce,
                const std::array<uint8_t, SERVER_ID_SIZE>* expectedServerId = nullptr) const;
};

/**
 * @brief JoinRequest - Client's request to join with invite
 * 
 * Contains:
 * - Serialized invite token
 * - Timestamp for replay protection
 * - Optional username hint
 */
struct JoinRequest {
    std::vector<uint8_t> inviteToken;  // Serialized InviteToken
    int64_t timestamp;                  // Unix timestamp
    std::string usernameHint;           // Requested username (server may assign different)
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<JoinRequest> Parse(const std::vector<uint8_t>& data);
};

/**
 * @brief JoinResponse - Server's response to join request
 * 
 * On success, contains session information.
 * On failure, contains error details.
 */
struct JoinResponse {
    JoinStatus status;
    
    // On success:
    uint64_t assignedUserId;
    std::string assignedUsername;
    std::vector<uint8_t> sessionToken;  // For subsequent authentication
    uint64_t permissions;               // Granted permissions
    
    // Server info
    std::string serverName;
    std::string serverDescription;
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<JoinResponse> Parse(const std::vector<uint8_t>& data);
};

//=============================================================================
// HANDSHAKE STATE MACHINES
//=============================================================================

/**
 * @brief Client-side handshake state machine
 * 
 * Usage:
 *   1. Create with expected server ID
 *   2. Call CreateClientHello() and send to server
 *   3. Receive ServerHello, call ProcessServerHello()
 *   4. If verified, call CreateJoinRequest() with invite token
 *   5. Receive JoinResponse, call ProcessJoinResponse()
 */
class ClientHandshake {
public:
    enum class State {
        Initial,
        WaitingForServerHello,
        ServerVerified,
        WaitingForJoinResponse,
        Completed,
        Failed
    };
    
    /**
     * @brief Create a client handshake
     * @param expectedServerId Optional: Server ID to verify (null = accept any)
     */
    explicit ClientHandshake(const std::array<uint8_t, SERVER_ID_SIZE>* expectedServerId = nullptr);
    
    /**
     * @brief Create ClientHello message
     */
    std::vector<uint8_t> CreateClientHello();
    
    /**
     * @brief Process received ServerHello
     * @return true if server is verified
     */
    bool ProcessServerHello(const std::vector<uint8_t>& data);
    
    /**
     * @brief Create JoinRequest message
     * @param inviteToken The invite token to present
     * @param usernameHint Requested username
     */
    std::vector<uint8_t> CreateJoinRequest(const InviteToken& inviteToken,
                                           const std::string& usernameHint = "");
    
    /**
     * @brief Process received JoinResponse
     * @return true if join was successful
     */
    bool ProcessJoinResponse(const std::vector<uint8_t>& data);
    
    // State accessors
    State GetState() const { return state; }
    JoinStatus GetLastError() const { return lastError; }
    const JoinResponse& GetJoinResult() const { return joinResult; }
    
    /**
     * @brief Get the verified server identity (after ProcessServerHello)
     */
    const std::optional<ServerIdentity>& GetVerifiedServer() const { return verifiedServer; }

private:
    State state;
    JoinStatus lastError;
    
    // Expected server (optional)
    std::optional<std::array<uint8_t, SERVER_ID_SIZE>> expectedServerId;
    
    // Handshake state
    std::array<uint8_t, NONCE_SIZE> clientNonce;
    std::array<uint8_t, NONCE_SIZE> serverNonce;
    std::optional<ServerIdentity> verifiedServer;
    
    // Result
    JoinResponse joinResult;
};

/**
 * @brief Server-side handshake handler
 * 
 * Usage:
 *   1. Create with server identity and invite manager
 *   2. Receive ClientHello, call ProcessClientHello()
 *   3. Get ServerHello and send to client
 *   4. Receive JoinRequest, call ProcessJoinRequest()
 *   5. Get JoinResponse and send to client
 */
class ServerHandshake {
public:
    enum class State {
        WaitingForClientHello,
        WaitingForJoinRequest,
        Completed,
        Failed
    };
    
    /**
     * @brief Create a server handshake handler
     * @param serverIdentity Server's cryptographic identity
     * @param inviteManager Manager for invite validation
     */
    ServerHandshake(const ServerIdentity& serverIdentity,
                    InviteManager& inviteManager);
    
    /**
     * @brief Process received ClientHello
     * @param data Received message
     * @return ServerHello to send (or empty on error)
     */
    std::vector<uint8_t> ProcessClientHello(const std::vector<uint8_t>& data);
    
    /**
     * @brief Process received JoinRequest
     * @param data Received message
     * @param assignUserIdCallback Callback to assign user ID
     * @return JoinResponse to send
     */
    std::vector<uint8_t> ProcessJoinRequest(
        const std::vector<uint8_t>& data,
        std::function<uint64_t(const std::string& username, InvitePermission perms)> assignUserCallback
    );
    
    // State accessors
    State GetState() const { return state; }
    JoinStatus GetLastError() const { return lastError; }

private:
    State state;
    JoinStatus lastError;
    
    const ServerIdentity& serverIdentity;
    InviteManager& inviteManager;
    
    // Handshake state
    std::array<uint8_t, NONCE_SIZE> clientNonce;
    std::array<uint8_t, NONCE_SIZE> serverNonce;
    std::optional<ServerHello> serverHello;
};

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

/**
 * @brief Wrap a handshake message with type header
 */
std::vector<uint8_t> WrapMessage(HandshakeMessageType type, const std::vector<uint8_t>& payload);

/**
 * @brief Unwrap a handshake message, returning type and payload
 */
std::optional<std::pair<HandshakeMessageType, std::vector<uint8_t>>> 
UnwrapMessage(const std::vector<uint8_t>& data);

} // namespace Security

#endif // SECURE_HANDSHAKE_H
