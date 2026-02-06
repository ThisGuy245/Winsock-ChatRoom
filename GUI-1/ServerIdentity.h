/**
 * @file ServerIdentity.h
 * @brief Cryptographic server identity using asymmetric key pairs
 * 
 * SECURITY ARCHITECTURE:
 * ----------------------
 * Each server has a unique cryptographic identity consisting of:
 * - A long-term ECDSA P-256 key pair (private key + public key)
 * - A Server ID derived from SHA-256(public_key)
 * 
 * THREAT MODEL:
 * - Server impersonation: Prevented by challenge-response authentication
 * - MITM attacks: Client verifies server signature before trusting
 * - IP hijacking: Server identity is independent of network address
 * 
 * WHY ECDSA P-256:
 * - Widely supported in Windows CNG (all Windows 10+ versions)
 * - 128-bit security level (sufficient for this application)
 * - Compact signatures (64 bytes)
 * - Fast verification
 * 
 * TRUST BOUNDARY:
 * - Private key MUST never leave the server
 * - Public key and Server ID can be freely shared
 * - Server proves identity by signing challenges with private key
 */

#ifndef SERVER_IDENTITY_H
#define SERVER_IDENTITY_H

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace Security {

// Key sizes for ECDSA P-256
constexpr size_t PUBLIC_KEY_SIZE = 65;      // Uncompressed point (1 + 32 + 32)
constexpr size_t PRIVATE_KEY_SIZE = 32;     // 256-bit scalar
constexpr size_t SIGNATURE_SIZE = 64;       // r (32 bytes) + s (32 bytes)
constexpr size_t SERVER_ID_SIZE = 32;       // SHA-256 output
constexpr size_t NONCE_SIZE = 32;           // Challenge nonce size
constexpr size_t SERVER_ID_DISPLAY_SIZE = 16; // Truncated for display (128 bits)

/**
 * @brief Result codes for cryptographic operations
 */
enum class CryptoResult {
    Success,
    KeyGenerationFailed,
    SigningFailed,
    VerificationFailed,
    InvalidKey,
    InvalidSignature,
    InvalidData,
    StorageError,
    NotInitialized
};

/**
 * @brief Convert CryptoResult to human-readable string
 */
const char* CryptoResultToString(CryptoResult result);

/**
 * @brief Represents a server's cryptographic identity
 * 
 * SECURITY NOTES:
 * - Private key is stored in memory and should be protected
 * - On destruction, private key memory is securely cleared
 * - Key pair should be saved to encrypted storage in production
 */
class ServerIdentity {
public:
    /**
     * @brief Generate a new server identity with fresh key pair
     * @return ServerIdentity instance or nullopt on failure
     * 
     * SECURITY: Uses Windows CNG BCryptGenRandom for key generation
     */
    static std::optional<ServerIdentity> Generate();
    
    /**
     * @brief Load server identity from file
     * @param filePath Path to identity file
     * @return ServerIdentity instance or nullopt on failure
     * 
     * SECURITY: File should be protected by filesystem permissions
     * TODO: Add encryption with password-derived key
     */
    static std::optional<ServerIdentity> LoadFromFile(const std::string& filePath);
    
    /**
     * @brief Create identity from existing keys (for verification only)
     * @param publicKey The public key bytes
     * @return ServerIdentity instance (signing not available)
     */
    static std::optional<ServerIdentity> FromPublicKey(
        const std::vector<uint8_t>& publicKey);
    
    // Destructor securely clears private key
    ~ServerIdentity();
    
    // Move semantics (no copying to prevent key duplication)
    ServerIdentity(ServerIdentity&& other) noexcept;
    ServerIdentity& operator=(ServerIdentity&& other) noexcept;
    ServerIdentity(const ServerIdentity&) = delete;
    ServerIdentity& operator=(const ServerIdentity&) = delete;
    
    /**
     * @brief Save identity to file
     * @param filePath Path to save identity
     * @return Success or error code
     * 
     * SECURITY: Only saves private key if this identity owns it
     */
    CryptoResult SaveToFile(const std::string& filePath) const;
    
    /**
     * @brief Sign arbitrary data with private key
     * @param data Data to sign
     * @param signature Output buffer for signature (64 bytes)
     * @return Success or error code
     * 
     * SECURITY: Requires private key (will fail for public-key-only instances)
     */
    CryptoResult Sign(const std::vector<uint8_t>& data,
                      std::array<uint8_t, SIGNATURE_SIZE>& signature) const;
    
    /**
     * @brief Verify a signature against this identity's public key
     * @param data Original data that was signed
     * @param signature Signature to verify (64 bytes)
     * @return Success if signature is valid, VerificationFailed otherwise
     */
    CryptoResult Verify(const std::vector<uint8_t>& data,
                        const std::array<uint8_t, SIGNATURE_SIZE>& signature) const;
    
    /**
     * @brief Sign a challenge nonce (for authentication)
     * @param challenge The nonce to sign
     * @param signature Output signature
     * @return Success or error code
     */
    CryptoResult SignChallenge(const std::array<uint8_t, NONCE_SIZE>& challenge,
                               std::array<uint8_t, SIGNATURE_SIZE>& signature) const;
    
    /**
     * @brief Verify a challenge response
     * @param challenge The original challenge
     * @param signature The response signature
     * @return Success if valid
     */
    CryptoResult VerifyChallenge(const std::array<uint8_t, NONCE_SIZE>& challenge,
                                 const std::array<uint8_t, SIGNATURE_SIZE>& signature) const;
    
    // Getters
    const std::array<uint8_t, SERVER_ID_SIZE>& GetServerId() const { return serverId; }
    const std::vector<uint8_t>& GetPublicKey() const { return publicKey; }
    bool HasPrivateKey() const { return hasPrivateKey; }
    
    /**
     * @brief Get Server ID as hex string for display
     * @param truncate If true, returns first 16 bytes (32 hex chars)
     */
    std::string GetServerIdHex(bool truncate = true) const;
    
    /**
     * @brief Verify that a Server ID matches a public key
     * @param publicKey The public key to check
     * @param serverId The expected Server ID
     * @return true if SHA-256(publicKey) == serverId
     */
    static bool VerifyServerIdMatchesKey(
        const std::vector<uint8_t>& publicKey,
        const std::array<uint8_t, SERVER_ID_SIZE>& serverId);

private:
    ServerIdentity() = default;
    
    std::array<uint8_t, SERVER_ID_SIZE> serverId;   // SHA-256(publicKey)
    std::vector<uint8_t> publicKey;                  // ECDSA P-256 public key
    std::vector<uint8_t> privateKey;                 // ECDSA P-256 private key (if owned)
    bool hasPrivateKey = false;
    
    // Compute Server ID from public key
    void ComputeServerId();
    
    // Secure memory clearing
    void SecureClearPrivateKey();
};

/**
 * @brief Generate cryptographically secure random bytes
 * @param buffer Output buffer
 * @param size Number of bytes to generate
 * @return Success or error code
 * 
 * SECURITY: Uses Windows CNG BCryptGenRandom (CSPRNG)
 */
CryptoResult GenerateRandomBytes(uint8_t* buffer, size_t size);

/**
 * @brief Generate a random nonce for challenges
 * @param nonce Output nonce
 * @return Success or error code
 */
CryptoResult GenerateNonce(std::array<uint8_t, NONCE_SIZE>& nonce);

/**
 * @brief Compute SHA-256 hash
 * @param data Input data
 * @param hash Output hash (32 bytes)
 * @return Success or error code
 */
CryptoResult ComputeSHA256(const std::vector<uint8_t>& data,
                           std::array<uint8_t, 32>& hash);

/**
 * @brief Convert bytes to hex string
 */
std::string BytesToHex(const uint8_t* data, size_t size);

/**
 * @brief Convert hex string to bytes
 * @return Empty vector on invalid input
 */
std::vector<uint8_t> HexToBytes(const std::string& hex);

} // namespace Security

#endif // SERVER_IDENTITY_H
