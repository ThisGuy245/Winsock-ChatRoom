/**
 * @file ServerIdentity.cpp
 * @brief Implementation of cryptographic server identity
 * 
 * IMPLEMENTATION NOTES:
 * - Uses Windows CNG (Cryptography API: Next Generation)
 * - ECDSA P-256 for signing (BCRYPT_ECDSA_P256_ALGORITHM)
 * - SHA-256 for hashing (BCRYPT_SHA256_ALGORITHM)
 * - BCryptGenRandom for secure random generation
 * 
 * SECURITY CONSIDERATIONS:
 * - Private keys are cleared from memory on destruction
 * - No logging of key material
 * - Signature verification uses constant-time comparison where possible
 */

#include "ServerIdentity.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <bcrypt.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

#pragma comment(lib, "bcrypt.lib")

namespace Security {

// Helper macro for CNG status checking
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

/**
 * @brief RAII wrapper for CNG algorithm handles
 */
class AlgorithmHandle {
public:
    AlgorithmHandle() : handle(nullptr) {}
    ~AlgorithmHandle() {
        if (handle) {
            BCryptCloseAlgorithmProvider(handle, 0);
        }
    }
    
    BCRYPT_ALG_HANDLE* operator&() { return &handle; }
    operator BCRYPT_ALG_HANDLE() { return handle; }
    
private:
    BCRYPT_ALG_HANDLE handle;
};

/**
 * @brief RAII wrapper for CNG key handles
 */
class KeyHandle {
public:
    KeyHandle() : handle(nullptr) {}
    ~KeyHandle() {
        if (handle) {
            BCryptDestroyKey(handle);
        }
    }
    
    BCRYPT_KEY_HANDLE* operator&() { return &handle; }
    operator BCRYPT_KEY_HANDLE() { return handle; }
    
private:
    BCRYPT_KEY_HANDLE handle;
};

/**
 * @brief RAII wrapper for CNG hash handles
 */
class HashHandle {
public:
    HashHandle() : handle(nullptr) {}
    ~HashHandle() {
        if (handle) {
            BCryptDestroyHash(handle);
        }
    }
    
    BCRYPT_HASH_HANDLE* operator&() { return &handle; }
    operator BCRYPT_HASH_HANDLE() { return handle; }
    
private:
    BCRYPT_HASH_HANDLE handle;
};

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

const char* CryptoResultToString(CryptoResult result) {
    switch (result) {
        case CryptoResult::Success: return "Success";
        case CryptoResult::KeyGenerationFailed: return "Key generation failed";
        case CryptoResult::SigningFailed: return "Signing failed";
        case CryptoResult::VerificationFailed: return "Verification failed";
        case CryptoResult::InvalidKey: return "Invalid key";
        case CryptoResult::InvalidSignature: return "Invalid signature";
        case CryptoResult::InvalidData: return "Invalid data";
        case CryptoResult::StorageError: return "Storage error";
        case CryptoResult::NotInitialized: return "Not initialized";
        default: return "Unknown error";
    }
}

std::string BytesToHex(const uint8_t* data, size_t size) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < size; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    
    if (hex.length() % 2 != 0) {
        return bytes; // Invalid length
    }
    
    bytes.reserve(hex.length() / 2);
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
            bytes.push_back(byte);
        } catch (...) {
            return std::vector<uint8_t>(); // Invalid hex
        }
    }
    
    return bytes;
}

CryptoResult GenerateRandomBytes(uint8_t* buffer, size_t size) {
    NTSTATUS status = BCryptGenRandom(
        nullptr,
        buffer,
        static_cast<ULONG>(size),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    
    return NT_SUCCESS(status) ? CryptoResult::Success : CryptoResult::KeyGenerationFailed;
}

CryptoResult GenerateNonce(std::array<uint8_t, NONCE_SIZE>& nonce) {
    return GenerateRandomBytes(nonce.data(), NONCE_SIZE);
}

CryptoResult ComputeSHA256(const std::vector<uint8_t>& data,
                           std::array<uint8_t, 32>& hash) {
    AlgorithmHandle algorithm;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm,
        BCRYPT_SHA256_ALGORITHM,
        nullptr,
        0
    );
    
    if (!NT_SUCCESS(status)) {
        return CryptoResult::InvalidData;
    }
    
    HashHandle hashHandle;
    status = BCryptCreateHash(algorithm, &hashHandle, nullptr, 0, nullptr, 0, 0);
    
    if (!NT_SUCCESS(status)) {
        return CryptoResult::InvalidData;
    }
    
    status = BCryptHashData(
        hashHandle,
        const_cast<uint8_t*>(data.data()),
        static_cast<ULONG>(data.size()),
        0
    );
    
    if (!NT_SUCCESS(status)) {
        return CryptoResult::InvalidData;
    }
    
    status = BCryptFinishHash(hashHandle, hash.data(), 32, 0);
    
    return NT_SUCCESS(status) ? CryptoResult::Success : CryptoResult::InvalidData;
}

//=============================================================================
// SERVER IDENTITY IMPLEMENTATION
//=============================================================================

ServerIdentity::~ServerIdentity() {
    SecureClearPrivateKey();
}

ServerIdentity::ServerIdentity(ServerIdentity&& other) noexcept
    : serverId(std::move(other.serverId))
    , publicKey(std::move(other.publicKey))
    , privateKey(std::move(other.privateKey))
    , hasPrivateKey(other.hasPrivateKey) {
    other.hasPrivateKey = false;
}

ServerIdentity& ServerIdentity::operator=(ServerIdentity&& other) noexcept {
    if (this != &other) {
        SecureClearPrivateKey();
        
        serverId = std::move(other.serverId);
        publicKey = std::move(other.publicKey);
        privateKey = std::move(other.privateKey);
        hasPrivateKey = other.hasPrivateKey;
        
        other.hasPrivateKey = false;
    }
    return *this;
}

void ServerIdentity::SecureClearPrivateKey() {
    if (!privateKey.empty()) {
        // Use volatile to prevent compiler optimization
        volatile uint8_t* ptr = privateKey.data();
        for (size_t i = 0; i < privateKey.size(); ++i) {
            ptr[i] = 0;
        }
        privateKey.clear();
    }
    hasPrivateKey = false;
}

void ServerIdentity::ComputeServerId() {
    ComputeSHA256(publicKey, serverId);
}

std::optional<ServerIdentity> ServerIdentity::Generate() {
    ServerIdentity identity;
    
    // Open ECDSA P-256 algorithm provider
    AlgorithmHandle algorithm;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm,
        BCRYPT_ECDSA_P256_ALGORITHM,
        nullptr,
        0
    );
    
    if (!NT_SUCCESS(status)) {
        printf("[CRYPTO] Failed to open ECDSA algorithm provider: 0x%08X\n", status);
        return std::nullopt;
    }
    
    // Generate key pair
    KeyHandle keyPair;
    status = BCryptGenerateKeyPair(algorithm, &keyPair, 256, 0);
    
    if (!NT_SUCCESS(status)) {
        printf("[CRYPTO] Failed to generate key pair: 0x%08X\n", status);
        return std::nullopt;
    }
    
    status = BCryptFinalizeKeyPair(keyPair, 0);
    
    if (!NT_SUCCESS(status)) {
        printf("[CRYPTO] Failed to finalize key pair: 0x%08X\n", status);
        return std::nullopt;
    }
    
    // Export public key
    ULONG publicKeySize = 0;
    status = BCryptExportKey(keyPair, nullptr, BCRYPT_ECCPUBLIC_BLOB, nullptr, 0, &publicKeySize, 0);
    
    if (!NT_SUCCESS(status)) {
        printf("[CRYPTO] Failed to get public key size: 0x%08X\n", status);
        return std::nullopt;
    }
    
    std::vector<uint8_t> publicKeyBlob(publicKeySize);
    status = BCryptExportKey(keyPair, nullptr, BCRYPT_ECCPUBLIC_BLOB, 
                             publicKeyBlob.data(), publicKeySize, &publicKeySize, 0);
    
    if (!NT_SUCCESS(status)) {
        printf("[CRYPTO] Failed to export public key: 0x%08X\n", status);
        return std::nullopt;
    }
    
    // Export private key
    ULONG privateKeySize = 0;
    status = BCryptExportKey(keyPair, nullptr, BCRYPT_ECCPRIVATE_BLOB, nullptr, 0, &privateKeySize, 0);
    
    if (!NT_SUCCESS(status)) {
        printf("[CRYPTO] Failed to get private key size: 0x%08X\n", status);
        return std::nullopt;
    }
    
    std::vector<uint8_t> privateKeyBlob(privateKeySize);
    status = BCryptExportKey(keyPair, nullptr, BCRYPT_ECCPRIVATE_BLOB,
                             privateKeyBlob.data(), privateKeySize, &privateKeySize, 0);
    
    if (!NT_SUCCESS(status)) {
        printf("[CRYPTO] Failed to export private key: 0x%08X\n", status);
        return std::nullopt;
    }
    
    identity.publicKey = std::move(publicKeyBlob);
    identity.privateKey = std::move(privateKeyBlob);
    identity.hasPrivateKey = true;
    identity.ComputeServerId();
    
    printf("[CRYPTO] Generated new server identity: %s\n", 
           identity.GetServerIdHex(true).c_str());
    
    return identity;
}

std::optional<ServerIdentity> ServerIdentity::FromPublicKey(
    const std::vector<uint8_t>& publicKey) {
    
    if (publicKey.empty()) {
        return std::nullopt;
    }
    
    ServerIdentity identity;
    identity.publicKey = publicKey;
    identity.hasPrivateKey = false;
    identity.ComputeServerId();
    
    return identity;
}

std::optional<ServerIdentity> ServerIdentity::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        printf("[CRYPTO] Failed to open identity file: %s\n", filePath.c_str());
        return std::nullopt;
    }
    
    ServerIdentity identity;
    
    // Read public key size and data
    uint32_t publicKeySize = 0;
    file.read(reinterpret_cast<char*>(&publicKeySize), sizeof(publicKeySize));
    
    if (publicKeySize == 0 || publicKeySize > 1024) {
        printf("[CRYPTO] Invalid public key size in identity file\n");
        return std::nullopt;
    }
    
    identity.publicKey.resize(publicKeySize);
    file.read(reinterpret_cast<char*>(identity.publicKey.data()), publicKeySize);
    
    // Read private key size and data
    uint32_t privateKeySize = 0;
    file.read(reinterpret_cast<char*>(&privateKeySize), sizeof(privateKeySize));
    
    if (privateKeySize > 0 && privateKeySize <= 1024) {
        identity.privateKey.resize(privateKeySize);
        file.read(reinterpret_cast<char*>(identity.privateKey.data()), privateKeySize);
        identity.hasPrivateKey = true;
    }
    
    if (!file) {
        printf("[CRYPTO] Failed to read identity file\n");
        return std::nullopt;
    }
    
    identity.ComputeServerId();
    
    printf("[CRYPTO] Loaded server identity: %s (has private key: %s)\n",
           identity.GetServerIdHex(true).c_str(),
           identity.hasPrivateKey ? "yes" : "no");
    
    return identity;
}

CryptoResult ServerIdentity::SaveToFile(const std::string& filePath) const {
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        return CryptoResult::StorageError;
    }
    
    // Write public key
    uint32_t publicKeySize = static_cast<uint32_t>(publicKey.size());
    file.write(reinterpret_cast<const char*>(&publicKeySize), sizeof(publicKeySize));
    file.write(reinterpret_cast<const char*>(publicKey.data()), publicKeySize);
    
    // Write private key (if we have it)
    uint32_t privateKeySize = static_cast<uint32_t>(privateKey.size());
    file.write(reinterpret_cast<const char*>(&privateKeySize), sizeof(privateKeySize));
    if (privateKeySize > 0) {
        file.write(reinterpret_cast<const char*>(privateKey.data()), privateKeySize);
    }
    
    if (!file) {
        return CryptoResult::StorageError;
    }
    
    printf("[CRYPTO] Saved server identity to: %s\n", filePath.c_str());
    return CryptoResult::Success;
}

CryptoResult ServerIdentity::Sign(const std::vector<uint8_t>& data,
                                  std::array<uint8_t, SIGNATURE_SIZE>& signature) const {
    if (!hasPrivateKey || privateKey.empty()) {
        return CryptoResult::NotInitialized;
    }
    
    // Open algorithm provider
    AlgorithmHandle algorithm;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm,
        BCRYPT_ECDSA_P256_ALGORITHM,
        nullptr,
        0
    );
    
    if (!NT_SUCCESS(status)) {
        return CryptoResult::SigningFailed;
    }
    
    // Import private key
    KeyHandle keyHandle;
    status = BCryptImportKeyPair(
        algorithm,
        nullptr,
        BCRYPT_ECCPRIVATE_BLOB,
        &keyHandle,
        const_cast<uint8_t*>(privateKey.data()),
        static_cast<ULONG>(privateKey.size()),
        0
    );
    
    if (!NT_SUCCESS(status)) {
        return CryptoResult::InvalidKey;
    }
    
    // Hash the data first (ECDSA signs hashes)
    std::array<uint8_t, 32> hash;
    CryptoResult hashResult = ComputeSHA256(data, hash);
    if (hashResult != CryptoResult::Success) {
        return hashResult;
    }
    
    // Sign the hash
    ULONG signatureSize = 0;
    status = BCryptSignHash(keyHandle, nullptr, hash.data(), 32, nullptr, 0, &signatureSize, 0);
    
    if (!NT_SUCCESS(status) || signatureSize != SIGNATURE_SIZE) {
        return CryptoResult::SigningFailed;
    }
    
    status = BCryptSignHash(keyHandle, nullptr, hash.data(), 32, 
                           signature.data(), SIGNATURE_SIZE, &signatureSize, 0);
    
    return NT_SUCCESS(status) ? CryptoResult::Success : CryptoResult::SigningFailed;
}

CryptoResult ServerIdentity::Verify(const std::vector<uint8_t>& data,
                                    const std::array<uint8_t, SIGNATURE_SIZE>& signature) const {
    if (publicKey.empty()) {
        return CryptoResult::NotInitialized;
    }
    
    // Open algorithm provider
    AlgorithmHandle algorithm;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithm,
        BCRYPT_ECDSA_P256_ALGORITHM,
        nullptr,
        0
    );
    
    if (!NT_SUCCESS(status)) {
        return CryptoResult::VerificationFailed;
    }
    
    // Import public key
    KeyHandle keyHandle;
    status = BCryptImportKeyPair(
        algorithm,
        nullptr,
        BCRYPT_ECCPUBLIC_BLOB,
        &keyHandle,
        const_cast<uint8_t*>(publicKey.data()),
        static_cast<ULONG>(publicKey.size()),
        0
    );
    
    if (!NT_SUCCESS(status)) {
        return CryptoResult::InvalidKey;
    }
    
    // Hash the data
    std::array<uint8_t, 32> hash;
    CryptoResult hashResult = ComputeSHA256(data, hash);
    if (hashResult != CryptoResult::Success) {
        return hashResult;
    }
    
    // Verify the signature
    status = BCryptVerifySignature(
        keyHandle,
        nullptr,
        hash.data(),
        32,
        const_cast<uint8_t*>(signature.data()),
        SIGNATURE_SIZE,
        0
    );
    
    return NT_SUCCESS(status) ? CryptoResult::Success : CryptoResult::VerificationFailed;
}

CryptoResult ServerIdentity::SignChallenge(
    const std::array<uint8_t, NONCE_SIZE>& challenge,
    std::array<uint8_t, SIGNATURE_SIZE>& signature) const {
    
    std::vector<uint8_t> data(challenge.begin(), challenge.end());
    return Sign(data, signature);
}

CryptoResult ServerIdentity::VerifyChallenge(
    const std::array<uint8_t, NONCE_SIZE>& challenge,
    const std::array<uint8_t, SIGNATURE_SIZE>& signature) const {
    
    std::vector<uint8_t> data(challenge.begin(), challenge.end());
    return Verify(data, signature);
}

std::string ServerIdentity::GetServerIdHex(bool truncate) const {
    size_t displaySize = truncate ? SERVER_ID_DISPLAY_SIZE : SERVER_ID_SIZE;
    return BytesToHex(serverId.data(), displaySize);
}

bool ServerIdentity::VerifyServerIdMatchesKey(
    const std::vector<uint8_t>& publicKey,
    const std::array<uint8_t, SERVER_ID_SIZE>& serverId) {
    
    std::array<uint8_t, 32> computedId;
    CryptoResult result = ComputeSHA256(publicKey, computedId);
    
    if (result != CryptoResult::Success) {
        return false;
    }
    
    // Constant-time comparison to prevent timing attacks
    volatile uint8_t diff = 0;
    for (size_t i = 0; i < SERVER_ID_SIZE; ++i) {
        diff |= computedId[i] ^ serverId[i];
    }
    
    return diff == 0;
}

} // namespace Security
