/**
 * @file InviteToken.cpp
 * @brief Implementation of cryptographic invite tokens
 */

#include "InviteToken.h"
#include "pugixml.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace Security {

//=============================================================================
// BASE64 ENCODING
//=============================================================================

static const char BASE64_CHARS[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64Encode(const std::vector<uint8_t>& data) {
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);
    
    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        
        if (i + 1 < data.size()) {
            n |= static_cast<uint32_t>(data[i + 1]) << 8;
        }
        if (i + 2 < data.size()) {
            n |= static_cast<uint32_t>(data[i + 2]);
        }
        
        result.push_back(BASE64_CHARS[(n >> 18) & 0x3F]);
        result.push_back(BASE64_CHARS[(n >> 12) & 0x3F]);
        
        if (i + 1 < data.size()) {
            result.push_back(BASE64_CHARS[(n >> 6) & 0x3F]);
        } else {
            result.push_back('=');
        }
        
        if (i + 2 < data.size()) {
            result.push_back(BASE64_CHARS[n & 0x3F]);
        } else {
            result.push_back('=');
        }
    }
    
    return result;
}

std::vector<uint8_t> Base64Decode(const std::string& encoded) {
    std::vector<uint8_t> result;
    
    if (encoded.empty() || encoded.size() % 4 != 0) {
        return result;
    }
    
    // Build reverse lookup
    int lookup[256] = {-1};
    for (int i = 0; i < 64; ++i) {
        lookup[static_cast<unsigned char>(BASE64_CHARS[i])] = i;
    }
    
    size_t outSize = encoded.size() / 4 * 3;
    if (encoded[encoded.size() - 1] == '=') outSize--;
    if (encoded[encoded.size() - 2] == '=') outSize--;
    
    result.reserve(outSize);
    
    for (size_t i = 0; i < encoded.size(); i += 4) {
        int n = 0;
        for (int j = 0; j < 4; ++j) {
            char c = encoded[i + j];
            if (c == '=') {
                n <<= 6;
            } else {
                int v = lookup[static_cast<unsigned char>(c)];
                if (v < 0) return std::vector<uint8_t>(); // Invalid character
                n = (n << 6) | v;
            }
        }
        
        result.push_back(static_cast<uint8_t>((n >> 16) & 0xFF));
        if (encoded[i + 2] != '=') {
            result.push_back(static_cast<uint8_t>((n >> 8) & 0xFF));
        }
        if (encoded[i + 3] != '=') {
            result.push_back(static_cast<uint8_t>(n & 0xFF));
        }
    }
    
    return result;
}

//=============================================================================
// TOKEN STATUS
//=============================================================================

const char* TokenStatusToString(TokenStatus status) {
    switch (status) {
        case TokenStatus::Valid: return "Valid";
        case TokenStatus::Expired: return "Token has expired";
        case TokenStatus::Exhausted: return "Token usage limit reached";
        case TokenStatus::Revoked: return "Token has been revoked";
        case TokenStatus::InvalidSignature: return "Invalid signature";
        case TokenStatus::WrongServer: return "Token is for a different server";
        case TokenStatus::Malformed: return "Token is malformed";
        default: return "Unknown status";
    }
}

//=============================================================================
// INVITE TOKEN IMPLEMENTATION
//=============================================================================

std::vector<uint8_t> InviteToken::GetSignableData() const {
    std::vector<uint8_t> data;
    
    // Reserve space for efficiency
    data.reserve(TOKEN_ID_SIZE + SERVER_ID_SIZE + 8 + 8 + 8 + 4 + 8);
    
    // Token ID
    data.insert(data.end(), tokenId.begin(), tokenId.end());
    
    // Server ID
    data.insert(data.end(), serverId.begin(), serverId.end());
    
    // Created by (uint64_t, little endian)
    for (int i = 0; i < 8; ++i) {
        data.push_back(static_cast<uint8_t>((createdBy >> (i * 8)) & 0xFF));
    }
    
    // Created at (time_t as int64_t)
    int64_t createdAtInt = static_cast<int64_t>(createdAt);
    for (int i = 0; i < 8; ++i) {
        data.push_back(static_cast<uint8_t>((createdAtInt >> (i * 8)) & 0xFF));
    }
    
    // Expires at (time_t as int64_t)
    int64_t expiresAtInt = static_cast<int64_t>(expiresAt);
    for (int i = 0; i < 8; ++i) {
        data.push_back(static_cast<uint8_t>((expiresAtInt >> (i * 8)) & 0xFF));
    }
    
    // Max uses (uint32_t)
    for (int i = 0; i < 4; ++i) {
        data.push_back(static_cast<uint8_t>((maxUses >> (i * 8)) & 0xFF));
    }
    
    // Permissions (uint64_t)
    uint64_t perms = static_cast<uint64_t>(permissions);
    for (int i = 0; i < 8; ++i) {
        data.push_back(static_cast<uint8_t>((perms >> (i * 8)) & 0xFF));
    }
    
    return data;
}

std::optional<InviteToken> InviteToken::Create(
    const ServerIdentity& serverIdentity,
    uint64_t createdBy,
    uint32_t expiresInSeconds,
    uint32_t maxUses,
    InvitePermission permissions) {
    
    if (!serverIdentity.HasPrivateKey()) {
        printf("[INVITE] Cannot create token: server identity has no private key\n");
        return std::nullopt;
    }
    
    InviteToken token;
    
    // Generate random token ID
    CryptoResult result = GenerateRandomBytes(token.tokenId.data(), TOKEN_ID_SIZE);
    if (result != CryptoResult::Success) {
        printf("[INVITE] Failed to generate token ID\n");
        return std::nullopt;
    }
    
    // Copy server ID
    token.serverId = serverIdentity.GetServerId();
    
    // Set metadata
    token.createdBy = createdBy;
    token.createdAt = std::time(nullptr);
    token.expiresAt = (expiresInSeconds > 0) ? token.createdAt + expiresInSeconds : 0;
    token.maxUses = maxUses;
    token.permissions = permissions;
    
    // Sign the token
    std::vector<uint8_t> signableData = token.GetSignableData();
    result = serverIdentity.Sign(signableData, token.signature);
    
    if (result != CryptoResult::Success) {
        printf("[INVITE] Failed to sign token: %s\n", CryptoResultToString(result));
        return std::nullopt;
    }
    
    printf("[INVITE] Created token %s (expires: %s, max uses: %u)\n",
           token.GetTokenIdHex().c_str(),
           token.expiresAt > 0 ? "yes" : "never",
           token.maxUses);
    
    return token;
}

std::optional<InviteToken> InviteToken::Parse(const std::vector<uint8_t>& data) {
    // Minimum size: tokenId + serverId + metadata + signature
    constexpr size_t MIN_SIZE = TOKEN_ID_SIZE + SERVER_ID_SIZE + 8 + 8 + 8 + 4 + 8 + SIGNATURE_SIZE;
    
    if (data.size() < MIN_SIZE) {
        printf("[INVITE] Token data too small: %zu bytes (need %zu)\n", data.size(), MIN_SIZE);
        return std::nullopt;
    }
    
    InviteToken token;
    size_t offset = 0;
    
    // Read token ID
    std::copy_n(data.begin() + offset, TOKEN_ID_SIZE, token.tokenId.begin());
    offset += TOKEN_ID_SIZE;
    
    // Read server ID
    std::copy_n(data.begin() + offset, SERVER_ID_SIZE, token.serverId.begin());
    offset += SERVER_ID_SIZE;
    
    // Read created by
    token.createdBy = 0;
    for (int i = 0; i < 8; ++i) {
        token.createdBy |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }
    offset += 8;
    
    // Read created at
    int64_t createdAtInt = 0;
    for (int i = 0; i < 8; ++i) {
        createdAtInt |= static_cast<int64_t>(data[offset + i]) << (i * 8);
    }
    token.createdAt = static_cast<std::time_t>(createdAtInt);
    offset += 8;
    
    // Read expires at
    int64_t expiresAtInt = 0;
    for (int i = 0; i < 8; ++i) {
        expiresAtInt |= static_cast<int64_t>(data[offset + i]) << (i * 8);
    }
    token.expiresAt = static_cast<std::time_t>(expiresAtInt);
    offset += 8;
    
    // Read max uses
    token.maxUses = 0;
    for (int i = 0; i < 4; ++i) {
        token.maxUses |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
    }
    offset += 4;
    
    // Read permissions
    uint64_t perms = 0;
    for (int i = 0; i < 8; ++i) {
        perms |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }
    token.permissions = static_cast<InvitePermission>(perms);
    offset += 8;
    
    // Read signature
    std::copy_n(data.begin() + offset, SIGNATURE_SIZE, token.signature.begin());
    
    return token;
}

std::optional<InviteToken> InviteToken::FromBase64(const std::string& encoded) {
    std::vector<uint8_t> data = Base64Decode(encoded);
    if (data.empty()) {
        return std::nullopt;
    }
    return Parse(data);
}

TokenStatus InviteToken::Validate(const ServerIdentity& serverIdentity) const {
    // Check server ID matches
    if (serverId != serverIdentity.GetServerId()) {
        return TokenStatus::WrongServer;
    }
    
    // Check expiration
    if (IsExpired()) {
        return TokenStatus::Expired;
    }
    
    // Verify signature
    std::vector<uint8_t> signableData = GetSignableData();
    CryptoResult result = serverIdentity.Verify(signableData, signature);
    
    if (result != CryptoResult::Success) {
        return TokenStatus::InvalidSignature;
    }
    
    return TokenStatus::Valid;
}

std::vector<uint8_t> InviteToken::Serialize() const {
    std::vector<uint8_t> data = GetSignableData();
    data.insert(data.end(), signature.begin(), signature.end());
    return data;
}

std::string InviteToken::ToBase64() const {
    return Base64Encode(Serialize());
}

std::string InviteToken::ToInviteLink(const std::string& baseUrl) const {
    return baseUrl + "invite/" + ToBase64();
}

std::string InviteToken::GetTokenIdHex() const {
    return BytesToHex(tokenId.data(), TOKEN_ID_SIZE);
}

bool InviteToken::IsExpired() const {
    if (expiresAt == 0) {
        return false; // Never expires
    }
    return std::time(nullptr) > expiresAt;
}

bool InviteToken::IsForServer(const ServerIdentity& server) const {
    return serverId == server.GetServerId();
}

bool InviteToken::IsForServer(const std::array<uint8_t, SERVER_ID_SIZE>& sid) const {
    return serverId == sid;
}

//=============================================================================
// INVITE MANAGER IMPLEMENTATION
//=============================================================================

InviteManager::InviteManager(const std::string& dataFilePath)
    : dataFilePath(dataFilePath) {
    LoadFromFile();
}

InviteManager::~InviteManager() {
    SaveToFile();
}

void InviteManager::LogAudit(const std::string& tokenIdHex, uint64_t userId, 
                             const std::string& action) {
    AuditEntry entry;
    entry.timestamp = std::time(nullptr);
    entry.tokenIdHex = tokenIdHex;
    entry.userId = userId;
    entry.action = action;
    auditLog.push_back(entry);
    
    printf("[AUDIT] Token %s: %s by user %llu\n", 
           tokenIdHex.c_str(), action.c_str(), userId);
}

std::optional<InviteToken> InviteManager::CreateInvite(
    const ServerIdentity& serverIdentity,
    uint64_t createdBy,
    uint32_t expiresInSeconds,
    uint32_t maxUses,
    InvitePermission permissions) {
    
    auto token = InviteToken::Create(serverIdentity, createdBy, 
                                     expiresInSeconds, maxUses, permissions);
    
    if (token) {
        // Track usage if limited
        if (maxUses > 0) {
            tokenUsage[token->GetTokenIdHex()] = maxUses;
        }
        
        LogAudit(token->GetTokenIdHex(), createdBy, "created");
        SaveToFile();
    }
    
    return token;
}

TokenStatus InviteManager::ValidateAndConsume(
    const InviteToken& token,
    const ServerIdentity& serverIdentity,
    uint64_t newUserId) {
    
    std::string tokenIdHex = token.GetTokenIdHex();
    
    // Check if revoked
    if (IsRevoked(token.GetTokenId())) {
        LogAudit(tokenIdHex, newUserId, "rejected (revoked)");
        return TokenStatus::Revoked;
    }
    
    // Check usage limit
    if (token.GetMaxUses() > 0) {
        auto it = tokenUsage.find(tokenIdHex);
        if (it != tokenUsage.end() && it->second == 0) {
            LogAudit(tokenIdHex, newUserId, "rejected (exhausted)");
            return TokenStatus::Exhausted;
        }
    }
    
    // Validate token cryptographically
    TokenStatus status = token.Validate(serverIdentity);
    if (status != TokenStatus::Valid) {
        LogAudit(tokenIdHex, newUserId, "rejected (" + std::string(TokenStatusToString(status)) + ")");
        return status;
    }
    
    // Consume one use
    if (token.GetMaxUses() > 0) {
        auto it = tokenUsage.find(tokenIdHex);
        if (it != tokenUsage.end() && it->second > 0) {
            it->second--;
        }
    }
    
    LogAudit(tokenIdHex, newUserId, "used");
    SaveToFile();
    
    return TokenStatus::Valid;
}

bool InviteManager::RevokeInvite(const std::array<uint8_t, TOKEN_ID_SIZE>& tokenId,
                                  uint64_t revokedBy) {
    std::string tokenIdHex = BytesToHex(tokenId.data(), TOKEN_ID_SIZE);
    
    // Check if already revoked
    if (revokedTokens.find(tokenIdHex) != revokedTokens.end()) {
        return false;
    }
    
    revokedTokens[tokenIdHex] = std::time(nullptr);
    LogAudit(tokenIdHex, revokedBy, "revoked");
    SaveToFile();
    
    return true;
}

bool InviteManager::IsRevoked(const std::array<uint8_t, TOKEN_ID_SIZE>& tokenId) const {
    std::string tokenIdHex = BytesToHex(tokenId.data(), TOKEN_ID_SIZE);
    return revokedTokens.find(tokenIdHex) != revokedTokens.end();
}

int InviteManager::GetRemainingUses(const std::array<uint8_t, TOKEN_ID_SIZE>& tokenId) const {
    std::string tokenIdHex = BytesToHex(tokenId.data(), TOKEN_ID_SIZE);
    auto it = tokenUsage.find(tokenIdHex);
    if (it == tokenUsage.end()) {
        return -1; // Not tracked (unlimited or unknown)
    }
    return static_cast<int>(it->second);
}

void InviteManager::SaveToFile() {
    pugi::xml_document doc;
    auto root = doc.append_child("InviteManager");
    
    // Save token usage
    auto usageNode = root.append_child("TokenUsage");
    for (const auto& pair : tokenUsage) {
        auto entry = usageNode.append_child("Token");
        entry.append_attribute("id") = pair.first.c_str();
        entry.append_attribute("remaining") = pair.second;
    }
    
    // Save revoked tokens
    auto revokedNode = root.append_child("RevokedTokens");
    for (const auto& pair : revokedTokens) {
        auto entry = revokedNode.append_child("Token");
        entry.append_attribute("id") = pair.first.c_str();
        entry.append_attribute("revokedAt") = static_cast<long long>(pair.second);
    }
    
    // Save audit log (last 1000 entries)
    auto auditNode = root.append_child("AuditLog");
    size_t startIdx = auditLog.size() > 1000 ? auditLog.size() - 1000 : 0;
    for (size_t i = startIdx; i < auditLog.size(); ++i) {
        const auto& entry = auditLog[i];
        auto entryNode = auditNode.append_child("Entry");
        entryNode.append_attribute("timestamp") = static_cast<long long>(entry.timestamp);
        entryNode.append_attribute("tokenId") = entry.tokenIdHex.c_str();
        entryNode.append_attribute("userId") = entry.userId;
        entryNode.append_attribute("action") = entry.action.c_str();
    }
    
    doc.save_file(dataFilePath.c_str());
}

void InviteManager::LoadFromFile() {
    pugi::xml_document doc;
    if (!doc.load_file(dataFilePath.c_str())) {
        return; // File doesn't exist yet
    }
    
    auto root = doc.child("InviteManager");
    if (!root) return;
    
    // Load token usage
    tokenUsage.clear();
    for (auto entry : root.child("TokenUsage").children("Token")) {
        std::string id = entry.attribute("id").as_string();
        uint32_t remaining = entry.attribute("remaining").as_uint();
        tokenUsage[id] = remaining;
    }
    
    // Load revoked tokens
    revokedTokens.clear();
    for (auto entry : root.child("RevokedTokens").children("Token")) {
        std::string id = entry.attribute("id").as_string();
        std::time_t revokedAt = entry.attribute("revokedAt").as_llong();
        revokedTokens[id] = revokedAt;
    }
    
    // Load audit log
    auditLog.clear();
    for (auto entry : root.child("AuditLog").children("Entry")) {
        AuditEntry ae;
        ae.timestamp = entry.attribute("timestamp").as_llong();
        ae.tokenIdHex = entry.attribute("tokenId").as_string();
        ae.userId = entry.attribute("userId").as_ullong();
        ae.action = entry.attribute("action").as_string();
        auditLog.push_back(ae);
    }
    
    printf("[INVITE] Loaded %zu tokens, %zu revoked, %zu audit entries\n",
           tokenUsage.size(), revokedTokens.size(), auditLog.size());
}

} // namespace Security
