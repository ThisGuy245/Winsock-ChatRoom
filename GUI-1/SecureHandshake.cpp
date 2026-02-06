/**
 * @file SecureHandshake.cpp
 * @brief Implementation of secure handshake protocol
 */

#include "SecureHandshake.h"
#include <cstring>
#include <algorithm>

namespace Security {

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

const char* JoinStatusToString(JoinStatus status) {
    switch (status) {
        case JoinStatus::Success: return "Success";
        case JoinStatus::InvalidInvite: return "Invalid invite token";
        case JoinStatus::InviteExpired: return "Invite has expired";
        case JoinStatus::InviteExhausted: return "Invite usage limit reached";
        case JoinStatus::InviteRevoked: return "Invite has been revoked";
        case JoinStatus::ServerFull: return "Server is full";
        case JoinStatus::Banned: return "You are banned from this server";
        case JoinStatus::VersionMismatch: return "Protocol version mismatch";
        case JoinStatus::InvalidMessage: return "Invalid message format";
        case JoinStatus::SignatureInvalid: return "Signature verification failed";
        case JoinStatus::TimestampInvalid: return "Timestamp out of acceptable range";
        case JoinStatus::ServerIdMismatch: return "Server identity does not match expected";
        case JoinStatus::ConnectionFailed: return "Connection failed";
        case JoinStatus::Timeout: return "Handshake timeout";
        default: return "Unknown error";
    }
}

// Helper to append bytes to a vector
static void AppendBytes(std::vector<uint8_t>& vec, const uint8_t* data, size_t size) {
    vec.insert(vec.end(), data, data + size);
}

// Helper to append a string with length prefix
static void AppendString(std::vector<uint8_t>& vec, const std::string& str) {
    uint16_t len = static_cast<uint16_t>(std::min(str.size(), size_t(65535)));
    vec.push_back(static_cast<uint8_t>(len & 0xFF));
    vec.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    AppendBytes(vec, reinterpret_cast<const uint8_t*>(str.data()), len);
}

// Helper to read a string with length prefix
static bool ReadString(const std::vector<uint8_t>& data, size_t& offset, std::string& out) {
    if (offset + 2 > data.size()) return false;
    
    uint16_t len = static_cast<uint16_t>(data[offset]) | 
                   (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;
    
    if (offset + len > data.size()) return false;
    
    out.assign(reinterpret_cast<const char*>(data.data() + offset), len);
    offset += len;
    return true;
}

std::vector<uint8_t> WrapMessage(HandshakeMessageType type, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> result;
    result.reserve(1 + 4 + payload.size());
    
    // Message type
    result.push_back(static_cast<uint8_t>(type));
    
    // Payload length (4 bytes, little endian)
    uint32_t len = static_cast<uint32_t>(payload.size());
    result.push_back(static_cast<uint8_t>(len & 0xFF));
    result.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
    
    // Payload
    AppendBytes(result, payload.data(), payload.size());
    
    return result;
}

std::optional<std::pair<HandshakeMessageType, std::vector<uint8_t>>> 
UnwrapMessage(const std::vector<uint8_t>& data) {
    if (data.size() < 5) {
        return std::nullopt;
    }
    
    HandshakeMessageType type = static_cast<HandshakeMessageType>(data[0]);
    
    uint32_t len = static_cast<uint32_t>(data[1]) |
                   (static_cast<uint32_t>(data[2]) << 8) |
                   (static_cast<uint32_t>(data[3]) << 16) |
                   (static_cast<uint32_t>(data[4]) << 24);
    
    if (data.size() < 5 + len) {
        return std::nullopt;
    }
    
    std::vector<uint8_t> payload(data.begin() + 5, data.begin() + 5 + len);
    
    return std::make_pair(type, payload);
}

//=============================================================================
// CLIENT HELLO
//=============================================================================

std::vector<uint8_t> ClientHello::Serialize() const {
    std::vector<uint8_t> data;
    data.reserve(2 + NONCE_SIZE);
    
    // Protocol version (2 bytes)
    data.push_back(static_cast<uint8_t>(protocolVersion & 0xFF));
    data.push_back(static_cast<uint8_t>((protocolVersion >> 8) & 0xFF));
    
    // Client nonce
    AppendBytes(data, clientNonce.data(), NONCE_SIZE);
    
    return WrapMessage(HandshakeMessageType::ClientHello, data);
}

std::optional<ClientHello> ClientHello::Parse(const std::vector<uint8_t>& data) {
    auto unwrapped = UnwrapMessage(data);
    if (!unwrapped || unwrapped->first != HandshakeMessageType::ClientHello) {
        return std::nullopt;
    }
    
    const auto& payload = unwrapped->second;
    if (payload.size() < 2 + NONCE_SIZE) {
        return std::nullopt;
    }
    
    ClientHello hello;
    hello.protocolVersion = static_cast<uint16_t>(payload[0]) |
                           (static_cast<uint16_t>(payload[1]) << 8);
    
    std::copy_n(payload.begin() + 2, NONCE_SIZE, hello.clientNonce.begin());
    
    return hello;
}

//=============================================================================
// SERVER HELLO
//=============================================================================

std::vector<uint8_t> ServerHello::Serialize() const {
    std::vector<uint8_t> data;
    
    // Server ID
    AppendBytes(data, serverId.data(), SERVER_ID_SIZE);
    
    // Public key with length prefix
    uint16_t pkLen = static_cast<uint16_t>(publicKey.size());
    data.push_back(static_cast<uint8_t>(pkLen & 0xFF));
    data.push_back(static_cast<uint8_t>((pkLen >> 8) & 0xFF));
    AppendBytes(data, publicKey.data(), publicKey.size());
    
    // Server nonce
    AppendBytes(data, serverNonce.data(), NONCE_SIZE);
    
    // Signature
    AppendBytes(data, signature.data(), SIGNATURE_SIZE);
    
    return WrapMessage(HandshakeMessageType::ServerHello, data);
}

std::optional<ServerHello> ServerHello::Parse(const std::vector<uint8_t>& data) {
    auto unwrapped = UnwrapMessage(data);
    if (!unwrapped || unwrapped->first != HandshakeMessageType::ServerHello) {
        return std::nullopt;
    }
    
    const auto& payload = unwrapped->second;
    size_t offset = 0;
    
    // Server ID
    if (offset + SERVER_ID_SIZE > payload.size()) return std::nullopt;
    ServerHello hello;
    std::copy_n(payload.begin() + offset, SERVER_ID_SIZE, hello.serverId.begin());
    offset += SERVER_ID_SIZE;
    
    // Public key length
    if (offset + 2 > payload.size()) return std::nullopt;
    uint16_t pkLen = static_cast<uint16_t>(payload[offset]) |
                    (static_cast<uint16_t>(payload[offset + 1]) << 8);
    offset += 2;
    
    // Public key
    if (offset + pkLen > payload.size()) return std::nullopt;
    hello.publicKey.assign(payload.begin() + offset, payload.begin() + offset + pkLen);
    offset += pkLen;
    
    // Server nonce
    if (offset + NONCE_SIZE > payload.size()) return std::nullopt;
    std::copy_n(payload.begin() + offset, NONCE_SIZE, hello.serverNonce.begin());
    offset += NONCE_SIZE;
    
    // Signature
    if (offset + SIGNATURE_SIZE > payload.size()) return std::nullopt;
    std::copy_n(payload.begin() + offset, SIGNATURE_SIZE, hello.signature.begin());
    
    return hello;
}

std::optional<ServerHello> ServerHello::Create(
    const ServerIdentity& serverIdentity,
    const std::array<uint8_t, NONCE_SIZE>& clientNonce) {
    
    if (!serverIdentity.HasPrivateKey()) {
        return std::nullopt;
    }
    
    ServerHello hello;
    hello.serverId = serverIdentity.GetServerId();
    hello.publicKey = serverIdentity.GetPublicKey();
    
    // Generate server nonce
    if (GenerateNonce(hello.serverNonce) != CryptoResult::Success) {
        return std::nullopt;
    }
    
    // Create data to sign: client_nonce || server_nonce
    std::vector<uint8_t> dataToSign;
    dataToSign.insert(dataToSign.end(), clientNonce.begin(), clientNonce.end());
    dataToSign.insert(dataToSign.end(), hello.serverNonce.begin(), hello.serverNonce.end());
    
    // Sign
    if (serverIdentity.Sign(dataToSign, hello.signature) != CryptoResult::Success) {
        return std::nullopt;
    }
    
    return hello;
}

bool ServerHello::Verify(const std::array<uint8_t, NONCE_SIZE>& clientNonce,
                         const std::array<uint8_t, SERVER_ID_SIZE>* expectedServerId) const {
    // First, verify server ID matches public key
    if (!ServerIdentity::VerifyServerIdMatchesKey(publicKey, serverId)) {
        printf("[HANDSHAKE] Server ID does not match public key\n");
        return false;
    }
    
    // Check expected server ID if provided
    if (expectedServerId && *expectedServerId != serverId) {
        printf("[HANDSHAKE] Server ID does not match expected\n");
        return false;
    }
    
    // Create identity from public key for verification
    auto identity = ServerIdentity::FromPublicKey(publicKey);
    if (!identity) {
        printf("[HANDSHAKE] Failed to create identity from public key\n");
        return false;
    }
    
    // Verify signature over client_nonce || server_nonce
    std::vector<uint8_t> dataToVerify;
    dataToVerify.insert(dataToVerify.end(), clientNonce.begin(), clientNonce.end());
    dataToVerify.insert(dataToVerify.end(), serverNonce.begin(), serverNonce.end());
    
    CryptoResult result = identity->Verify(dataToVerify, signature);
    if (result != CryptoResult::Success) {
        printf("[HANDSHAKE] Signature verification failed\n");
        return false;
    }
    
    return true;
}

//=============================================================================
// JOIN REQUEST
//=============================================================================

std::vector<uint8_t> JoinRequest::Serialize() const {
    std::vector<uint8_t> data;
    
    // Invite token length and data
    uint32_t tokenLen = static_cast<uint32_t>(inviteToken.size());
    data.push_back(static_cast<uint8_t>(tokenLen & 0xFF));
    data.push_back(static_cast<uint8_t>((tokenLen >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((tokenLen >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((tokenLen >> 24) & 0xFF));
    AppendBytes(data, inviteToken.data(), inviteToken.size());
    
    // Timestamp (8 bytes)
    for (int i = 0; i < 8; ++i) {
        data.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
    }
    
    // Username hint
    AppendString(data, usernameHint);
    
    return WrapMessage(HandshakeMessageType::JoinRequest, data);
}

std::optional<JoinRequest> JoinRequest::Parse(const std::vector<uint8_t>& data) {
    auto unwrapped = UnwrapMessage(data);
    if (!unwrapped || unwrapped->first != HandshakeMessageType::JoinRequest) {
        return std::nullopt;
    }
    
    const auto& payload = unwrapped->second;
    size_t offset = 0;
    
    // Token length
    if (offset + 4 > payload.size()) return std::nullopt;
    uint32_t tokenLen = static_cast<uint32_t>(payload[offset]) |
                       (static_cast<uint32_t>(payload[offset + 1]) << 8) |
                       (static_cast<uint32_t>(payload[offset + 2]) << 16) |
                       (static_cast<uint32_t>(payload[offset + 3]) << 24);
    offset += 4;
    
    // Token data
    if (offset + tokenLen > payload.size()) return std::nullopt;
    JoinRequest req;
    req.inviteToken.assign(payload.begin() + offset, payload.begin() + offset + tokenLen);
    offset += tokenLen;
    
    // Timestamp
    if (offset + 8 > payload.size()) return std::nullopt;
    req.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        req.timestamp |= static_cast<int64_t>(payload[offset + i]) << (i * 8);
    }
    offset += 8;
    
    // Username hint
    if (!ReadString(payload, offset, req.usernameHint)) {
        req.usernameHint = "";
    }
    
    return req;
}

//=============================================================================
// JOIN RESPONSE
//=============================================================================

std::vector<uint8_t> JoinResponse::Serialize() const {
    std::vector<uint8_t> data;
    
    // Status
    data.push_back(static_cast<uint8_t>(status));
    
    if (status == JoinStatus::Success) {
        // User ID
        for (int i = 0; i < 8; ++i) {
            data.push_back(static_cast<uint8_t>((assignedUserId >> (i * 8)) & 0xFF));
        }
        
        // Username
        AppendString(data, assignedUsername);
        
        // Session token length and data
        uint16_t tokenLen = static_cast<uint16_t>(sessionToken.size());
        data.push_back(static_cast<uint8_t>(tokenLen & 0xFF));
        data.push_back(static_cast<uint8_t>((tokenLen >> 8) & 0xFF));
        AppendBytes(data, sessionToken.data(), sessionToken.size());
        
        // Permissions
        for (int i = 0; i < 8; ++i) {
            data.push_back(static_cast<uint8_t>((permissions >> (i * 8)) & 0xFF));
        }
        
        // Server info
        AppendString(data, serverName);
        AppendString(data, serverDescription);
    }
    
    return WrapMessage(HandshakeMessageType::JoinResponse, data);
}

std::optional<JoinResponse> JoinResponse::Parse(const std::vector<uint8_t>& data) {
    auto unwrapped = UnwrapMessage(data);
    if (!unwrapped || unwrapped->first != HandshakeMessageType::JoinResponse) {
        return std::nullopt;
    }
    
    const auto& payload = unwrapped->second;
    if (payload.empty()) return std::nullopt;
    
    JoinResponse resp;
    resp.status = static_cast<JoinStatus>(payload[0]);
    
    if (resp.status == JoinStatus::Success) {
        size_t offset = 1;
        
        // User ID
        if (offset + 8 > payload.size()) return std::nullopt;
        resp.assignedUserId = 0;
        for (int i = 0; i < 8; ++i) {
            resp.assignedUserId |= static_cast<uint64_t>(payload[offset + i]) << (i * 8);
        }
        offset += 8;
        
        // Username
        if (!ReadString(payload, offset, resp.assignedUsername)) return std::nullopt;
        
        // Session token
        if (offset + 2 > payload.size()) return std::nullopt;
        uint16_t tokenLen = static_cast<uint16_t>(payload[offset]) |
                          (static_cast<uint16_t>(payload[offset + 1]) << 8);
        offset += 2;
        
        if (offset + tokenLen > payload.size()) return std::nullopt;
        resp.sessionToken.assign(payload.begin() + offset, payload.begin() + offset + tokenLen);
        offset += tokenLen;
        
        // Permissions
        if (offset + 8 > payload.size()) return std::nullopt;
        resp.permissions = 0;
        for (int i = 0; i < 8; ++i) {
            resp.permissions |= static_cast<uint64_t>(payload[offset + i]) << (i * 8);
        }
        offset += 8;
        
        // Server info
        ReadString(payload, offset, resp.serverName);
        ReadString(payload, offset, resp.serverDescription);
    }
    
    return resp;
}

//=============================================================================
// CLIENT HANDSHAKE STATE MACHINE
//=============================================================================

ClientHandshake::ClientHandshake(const std::array<uint8_t, SERVER_ID_SIZE>* expectedServerId)
    : state(State::Initial)
    , lastError(JoinStatus::Success) {
    
    if (expectedServerId) {
        this->expectedServerId = *expectedServerId;
    }
}

std::vector<uint8_t> ClientHandshake::CreateClientHello() {
    // Generate client nonce
    if (GenerateNonce(clientNonce) != CryptoResult::Success) {
        state = State::Failed;
        lastError = JoinStatus::ConnectionFailed;
        return {};
    }
    
    ClientHello hello;
    hello.protocolVersion = PROTOCOL_VERSION;
    hello.clientNonce = clientNonce;
    
    state = State::WaitingForServerHello;
    return hello.Serialize();
}

bool ClientHandshake::ProcessServerHello(const std::vector<uint8_t>& data) {
    if (state != State::WaitingForServerHello) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        return false;
    }
    
    auto hello = ServerHello::Parse(data);
    if (!hello) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        return false;
    }
    
    // Verify server identity
    const std::array<uint8_t, SERVER_ID_SIZE>* expected = 
        expectedServerId ? &(*expectedServerId) : nullptr;
    
    if (!hello->Verify(clientNonce, expected)) {
        lastError = expectedServerId ? JoinStatus::ServerIdMismatch : JoinStatus::SignatureInvalid;
        state = State::Failed;
        return false;
    }
    
    // Store verified server info
    serverNonce = hello->serverNonce;
    verifiedServer = ServerIdentity::FromPublicKey(hello->publicKey);
    
    state = State::ServerVerified;
    printf("[HANDSHAKE] Server identity verified: %s\n", 
           BytesToHex(hello->serverId.data(), SERVER_ID_DISPLAY_SIZE).c_str());
    
    return true;
}

std::vector<uint8_t> ClientHandshake::CreateJoinRequest(const InviteToken& inviteToken,
                                                        const std::string& usernameHint) {
    if (state != State::ServerVerified) {
        lastError = JoinStatus::InvalidMessage;
        return {};
    }
    
    JoinRequest req;
    req.inviteToken = inviteToken.Serialize();
    req.timestamp = static_cast<int64_t>(std::time(nullptr));
    req.usernameHint = usernameHint;
    
    state = State::WaitingForJoinResponse;
    return req.Serialize();
}

bool ClientHandshake::ProcessJoinResponse(const std::vector<uint8_t>& data) {
    if (state != State::WaitingForJoinResponse) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        return false;
    }
    
    auto resp = JoinResponse::Parse(data);
    if (!resp) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        return false;
    }
    
    joinResult = *resp;
    lastError = resp->status;
    
    if (resp->status == JoinStatus::Success) {
        state = State::Completed;
        printf("[HANDSHAKE] Join successful: user %llu (%s)\n",
               joinResult.assignedUserId, joinResult.assignedUsername.c_str());
        return true;
    } else {
        state = State::Failed;
        printf("[HANDSHAKE] Join failed: %s\n", JoinStatusToString(resp->status));
        return false;
    }
}

//=============================================================================
// SERVER HANDSHAKE STATE MACHINE
//=============================================================================

ServerHandshake::ServerHandshake(const ServerIdentity& serverIdentity,
                                 InviteManager& inviteManager)
    : state(State::WaitingForClientHello)
    , lastError(JoinStatus::Success)
    , serverIdentity(serverIdentity)
    , inviteManager(inviteManager) {
}

std::vector<uint8_t> ServerHandshake::ProcessClientHello(const std::vector<uint8_t>& data) {
    if (state != State::WaitingForClientHello) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        return {};
    }
    
    auto hello = ClientHello::Parse(data);
    if (!hello) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        return {};
    }
    
    // Check protocol version
    if (hello->protocolVersion != PROTOCOL_VERSION) {
        lastError = JoinStatus::VersionMismatch;
        state = State::Failed;
        return {};
    }
    
    clientNonce = hello->clientNonce;
    
    // Create ServerHello
    auto serverHelloOpt = ServerHello::Create(serverIdentity, clientNonce);
    if (!serverHelloOpt) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        return {};
    }
    
    serverHello = *serverHelloOpt;
    serverNonce = serverHello->serverNonce;
    
    state = State::WaitingForJoinRequest;
    return serverHello->Serialize();
}

std::vector<uint8_t> ServerHandshake::ProcessJoinRequest(
    const std::vector<uint8_t>& data,
    std::function<uint64_t(const std::string& username, InvitePermission perms)> assignUserCallback) {
    
    if (state != State::WaitingForJoinRequest) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        JoinResponse resp;
        resp.status = JoinStatus::InvalidMessage;
        return resp.Serialize();
    }
    
    auto req = JoinRequest::Parse(data);
    if (!req) {
        lastError = JoinStatus::InvalidMessage;
        state = State::Failed;
        JoinResponse resp;
        resp.status = JoinStatus::InvalidMessage;
        return resp.Serialize();
    }
    
    // Validate timestamp
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    int64_t skew = std::abs(now - req->timestamp);
    if (skew > MAX_CLOCK_SKEW) {
        lastError = JoinStatus::TimestampInvalid;
        state = State::Failed;
        JoinResponse resp;
        resp.status = JoinStatus::TimestampInvalid;
        return resp.Serialize();
    }
    
    // Parse and validate invite token
    auto token = InviteToken::Parse(req->inviteToken);
    if (!token) {
        lastError = JoinStatus::InvalidInvite;
        state = State::Failed;
        JoinResponse resp;
        resp.status = JoinStatus::InvalidInvite;
        return resp.Serialize();
    }
    
    // Use invite manager to validate and consume
    // We need a temporary user ID to pass - use 0 for now, real ID assigned later
    TokenStatus tokenStatus = inviteManager.ValidateAndConsume(*token, serverIdentity, 0);
    
    JoinResponse resp;
    
    switch (tokenStatus) {
        case TokenStatus::Valid:
            break;
        case TokenStatus::Expired:
            resp.status = JoinStatus::InviteExpired;
            lastError = resp.status;
            state = State::Failed;
            return resp.Serialize();
        case TokenStatus::Exhausted:
            resp.status = JoinStatus::InviteExhausted;
            lastError = resp.status;
            state = State::Failed;
            return resp.Serialize();
        case TokenStatus::Revoked:
            resp.status = JoinStatus::InviteRevoked;
            lastError = resp.status;
            state = State::Failed;
            return resp.Serialize();
        case TokenStatus::WrongServer:
            resp.status = JoinStatus::InvalidInvite;
            lastError = resp.status;
            state = State::Failed;
            return resp.Serialize();
        default:
            resp.status = JoinStatus::InvalidInvite;
            lastError = resp.status;
            state = State::Failed;
            return resp.Serialize();
    }
    
    // Invite is valid - assign user
    InvitePermission perms = token->GetPermissions();
    uint64_t userId = assignUserCallback(req->usernameHint, perms);
    
    // Generate session token
    std::vector<uint8_t> sessionToken(32);
    if (GenerateRandomBytes(sessionToken.data(), 32) != CryptoResult::Success) {
        resp.status = JoinStatus::InvalidMessage;
        lastError = resp.status;
        state = State::Failed;
        return resp.Serialize();
    }
    
    // Build success response
    resp.status = JoinStatus::Success;
    resp.assignedUserId = userId;
    resp.assignedUsername = req->usernameHint.empty() ? "User" + std::to_string(userId) : req->usernameHint;
    resp.sessionToken = sessionToken;
    resp.permissions = static_cast<uint64_t>(perms);
    resp.serverName = "Secure Server"; // Would come from server config
    resp.serverDescription = "";
    
    state = State::Completed;
    lastError = JoinStatus::Success;
    
    printf("[HANDSHAKE] User %llu (%s) joined successfully\n",
           resp.assignedUserId, resp.assignedUsername.c_str());
    
    return resp.Serialize();
}

} // namespace Security
