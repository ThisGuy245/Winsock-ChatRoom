#ifndef NETPROTOCOL_H
#define NETPROTOCOL_H

// Prevent Windows min/max macros from conflicting with std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif

/**
 * @file NetProtocol.h
 * @brief Secure message framing protocol for TCP communication
 * 
 * SECURITY PURPOSE:
 * TCP is a stream protocol with no built-in message boundaries.
 * A single recv() call may return:
 *   - Part of a message (network fragmentation)
 *   - Multiple messages concatenated (Nagle's algorithm, fast sender)
 *   - Exactly one message (luck)
 * 
 * THREAT MODEL:
 * Without proper framing, an attacker can:
 *   1. Cause message corruption by sending data faster than we process
 *   2. Inject commands by manipulating message boundaries
 *   3. Cause buffer overflows by sending oversized messages
 *   4. Cause denial of service by sending malformed length headers
 * 
 * SOLUTION:
 * Length-prefixed framing with explicit bounds checking.
 * Format: [4-byte length (network byte order)][payload]
 * 
 * @author Security hardening by Phase 1 implementation
 */

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>

namespace NetProtocol {

//=============================================================================
// PROTOCOL CONSTANTS
// These values define the wire protocol. Changing them breaks compatibility.
//=============================================================================

/**
 * @brief Maximum allowed message size in bytes
 * 
 * SECURITY RATIONALE:
 * - Prevents memory exhaustion attacks (attacker claims 4GB message)
 * - Chosen to be large enough for any reasonable chat message
 * - Small enough to detect obviously malicious length values
 * 
 * THREAT MITIGATED: Memory exhaustion DoS, integer overflow in allocation
 */
constexpr uint32_t MAX_MESSAGE_SIZE = 65536;  // 64 KB

/**
 * @brief Minimum message size (empty message is technically valid)
 */
constexpr uint32_t MIN_MESSAGE_SIZE = 0;

/**
 * @brief Size of the length prefix header in bytes
 */
constexpr size_t HEADER_SIZE = sizeof(uint32_t);

/**
 * @brief Timeout for blocking operations in milliseconds
 * 
 * SECURITY RATIONALE:
 * - Prevents indefinite blocking on slow/malicious clients
 * - Allows server to detect and disconnect unresponsive clients
 */
constexpr int RECV_TIMEOUT_MS = 30000;  // 30 seconds

//=============================================================================
// RESULT TYPES
// Explicit error handling - no silent failures
//=============================================================================

/**
 * @brief Result codes for network operations
 * 
 * DESIGN PRINCIPLE: Fail closed, not open.
 * Any ambiguous result should be treated as an error.
 */
enum class Result {
    Success,            ///< Operation completed successfully
    WouldBlock,         ///< No data available (non-blocking socket) - try again later
    Disconnected,       ///< Peer closed connection gracefully
    Timeout,            ///< Operation timed out
    MessageTooLarge,    ///< Message exceeds MAX_MESSAGE_SIZE
    InvalidLength,      ///< Received length header is invalid/suspicious
    NetworkError,       ///< Winsock error occurred (check WSAGetLastError)
    BufferError         ///< Memory allocation or buffer operation failed
};

/**
 * @brief Convert Result to human-readable string (for logging)
 */
const char* ResultToString(Result result);

//=============================================================================
// CORE PROTOCOL FUNCTIONS
// These are the ONLY functions that should touch raw sockets for data transfer
//=============================================================================

/**
 * @brief Send a complete message with length prefix
 * 
 * SECURITY GUARANTEES:
 * - Validates message size before sending
 * - Sends complete message atomically (from application perspective)
 * - Uses network byte order for cross-platform compatibility
 * 
 * @param socket    The connected socket to send on
 * @param message   The message payload to send
 * @return          Result::Success or error code
 * 
 * ATTACKER-CONTROLLED: The socket connection, network path
 * TRUSTED: The message content (caller's responsibility to sanitize)
 */
Result SendMessage(SOCKET socket, const std::string& message);

/**
 * @brief Receive a complete message with length prefix
 * 
 * SECURITY GUARANTEES:
 * - Validates length header before allocating memory
 * - Handles partial reads correctly (TCP fragmentation)
 * - Will not allocate more than MAX_MESSAGE_SIZE bytes
 * - Times out on slow/stalled connections
 * 
 * @param socket    The connected socket to receive from
 * @param message   Output: the received message (cleared on error)
 * @return          Result::Success or error code
 * 
 * ATTACKER-CONTROLLED: ALL data received from socket
 * The returned message must be validated before use.
 */
Result ReceiveMessage(SOCKET socket, std::string& message);

//=============================================================================
// LOW-LEVEL HELPERS
// Used internally; exposed for testing and edge cases
//=============================================================================

/**
 * @brief Read exactly n bytes from socket, handling partial reads
 * 
 * SECURITY NOTE:
 * TCP recv() can return fewer bytes than requested. This function
 * loops until exactly 'length' bytes are received or an error occurs.
 * 
 * @param socket    The connected socket
 * @param buffer    Pre-allocated buffer of at least 'length' bytes
 * @param length    Exact number of bytes to read
 * @return          Result::Success if exactly 'length' bytes read
 */
Result RecvExact(SOCKET socket, void* buffer, size_t length);

/**
 * @brief Send exactly n bytes to socket, handling partial sends
 * 
 * SECURITY NOTE:
 * send() can return fewer bytes than requested under memory pressure
 * or with non-blocking sockets. This function loops until complete.
 * 
 * @param socket    The connected socket
 * @param buffer    Data to send
 * @param length    Exact number of bytes to send
 * @return          Result::Success if all bytes sent
 */
Result SendExact(SOCKET socket, const void* buffer, size_t length);

/**
 * @brief Configure socket for secure operation
 * 
 * Sets appropriate timeouts and options for security.
 * Call this after accept() or connect().
 * 
 * @param socket    The socket to configure
 * @return          Result::Success if configuration applied
 */
Result ConfigureSocket(SOCKET socket);

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

/**
 * @brief Securely clear sensitive data from memory
 * 
 * SECURITY RATIONALE:
 * Prevents sensitive data (passwords, keys) from lingering in memory
 * where it could be recovered by memory dumps or other attacks.
 * 
 * Uses volatile to prevent compiler optimization removing the clear.
 * 
 * @param buffer    Pointer to buffer to clear
 * @param length    Number of bytes to clear
 */
void SecureClear(void* buffer, size_t length);

/**
 * @brief Securely clear a string's contents
 */
void SecureClear(std::string& str);

} // namespace NetProtocol

#endif // NETPROTOCOL_H
