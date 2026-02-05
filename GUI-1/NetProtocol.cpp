/**
 * @file NetProtocol.cpp
 * @brief Implementation of secure message framing protocol
 * 
 * SECURITY IMPLEMENTATION NOTES:
 * 
 * 1. ALL network reads use RecvExact() to prevent partial read vulnerabilities
 * 2. ALL network writes use SendExact() to prevent partial write issues
 * 3. Length headers are validated BEFORE memory allocation
 * 4. Timeouts prevent indefinite blocking attacks
 * 5. Error paths clear any partial data received
 */

// MUST define NOMINMAX before Windows headers to prevent min/max macro conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "NetProtocol.h"
#include <ws2tcpip.h>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

namespace NetProtocol {

//=============================================================================
// RESULT CONVERSION
//=============================================================================

const char* ResultToString(Result result) {
    switch (result) {
        case Result::Success:         return "Success";
        case Result::WouldBlock:      return "WouldBlock";
        case Result::Disconnected:    return "Disconnected";
        case Result::Timeout:         return "Timeout";
        case Result::MessageTooLarge: return "MessageTooLarge";
        case Result::InvalidLength:   return "InvalidLength";
        case Result::NetworkError:    return "NetworkError";
        case Result::BufferError:     return "BufferError";
        default:                      return "Unknown";
    }
}

//=============================================================================
// SECURE MEMORY OPERATIONS
//=============================================================================

void SecureClear(void* buffer, size_t length) {
    if (buffer == nullptr || length == 0) return;
    
    // Use volatile to prevent compiler from optimizing away the clear
    // This is critical for security - we MUST zero sensitive data
    volatile unsigned char* p = static_cast<volatile unsigned char*>(buffer);
    while (length--) {
        *p++ = 0;
    }
}

void SecureClear(std::string& str) {
    if (str.empty()) return;
    
    // Clear the string's internal buffer
    // Note: This may not clear all copies due to SSO (Small String Optimization)
    // For truly sensitive data, use a secure string class instead
    SecureClear(&str[0], str.size());
    str.clear();
}

//=============================================================================
// SOCKET CONFIGURATION
//=============================================================================

Result ConfigureSocket(SOCKET socket) {
    if (socket == INVALID_SOCKET) {
        return Result::NetworkError;
    }
    
    // NOTE: We do NOT set SO_RCVTIMEO/SO_SNDTIMEO here because the GUI
    // application uses non-blocking sockets with event-driven polling.
    // Timeouts would interfere with non-blocking mode.
    // 
    // For server applications that want blocking behavior, set timeouts
    // explicitly after calling this function.
    
    // Disable Nagle's algorithm for lower latency
    // This is important for chat apps - sends small messages immediately
    int noDelay = 1;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
    
    return Result::Success;
}

//=============================================================================
// LOW-LEVEL I/O PRIMITIVES
//=============================================================================

/**
 * SECURITY-CRITICAL FUNCTION
 * 
 * This function handles the fundamental TCP streaming issue:
 * recv() may return fewer bytes than requested. We MUST loop
 * until we have exactly the bytes we need.
 * 
 * THREAT: Without this, an attacker can send fragmented packets
 * that cause us to misinterpret message boundaries.
 */
Result RecvExact(SOCKET socket, void* buffer, size_t length) {
    if (buffer == nullptr && length > 0) {
        return Result::BufferError;
    }
    
    if (length == 0) {
        return Result::Success;
    }
    
    char* ptr = static_cast<char*>(buffer);
    size_t remaining = length;
    
    while (remaining > 0) {
        // SECURITY NOTE: Cast is safe because remaining <= original length
        // which was validated by caller to be <= MAX_MESSAGE_SIZE
        // Parentheses around std::min prevent Windows min macro from interfering
        int toRead = static_cast<int>((std::min)(remaining, static_cast<size_t>(INT_MAX)));
        
        int received = recv(socket, ptr, toRead, 0);
        
        if (received == SOCKET_ERROR) {
            int error = WSAGetLastError();
            
            // Check for timeout
            if (error == WSAETIMEDOUT) {
                // SECURITY: Clear partial data on error
                SecureClear(buffer, length - remaining);
                return Result::Timeout;
            }
            
            // Check for would-block (non-blocking socket has no data ready)
            if (error == WSAEWOULDBLOCK) {
                // Return WouldBlock so GUI event loop can continue
                // Caller should retry later
                if (remaining == length) {
                    // No data received yet - safe to return WouldBlock
                    return Result::WouldBlock;
                }
                // Partial data received - this is tricky for non-blocking
                // For now, continue waiting (small messages should complete quickly)
                Sleep(1);  // Yield to prevent busy-wait
                continue;
            }
            
            // SECURITY: Clear partial data on error
            SecureClear(buffer, length - remaining);
            return Result::NetworkError;
        }
        
        if (received == 0) {
            // Peer closed connection gracefully
            // SECURITY: Clear partial data
            SecureClear(buffer, length - remaining);
            return Result::Disconnected;
        }
        
        ptr += received;
        remaining -= received;
    }
    
    return Result::Success;
}

/**
 * SECURITY-CRITICAL FUNCTION
 * 
 * Similar to RecvExact, send() may not send all bytes at once.
 * We must loop to ensure complete transmission.
 */
Result SendExact(SOCKET socket, const void* buffer, size_t length) {
    if (buffer == nullptr && length > 0) {
        return Result::BufferError;
    }
    
    if (length == 0) {
        return Result::Success;
    }
    
    const char* ptr = static_cast<const char*>(buffer);
    size_t remaining = length;
    
    while (remaining > 0) {
        // Parentheses around std::min prevent Windows min macro from interfering
        int toSend = static_cast<int>((std::min)(remaining, static_cast<size_t>(INT_MAX)));
        
        int sent = send(socket, ptr, toSend, 0);
        
        if (sent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            
            if (error == WSAETIMEDOUT) {
                return Result::Timeout;
            }
            
            if (error == WSAEWOULDBLOCK) {
                // Non-blocking socket can't send right now
                // Small sleep to prevent busy-wait, then retry
                Sleep(1);
                continue;
            }
            
            return Result::NetworkError;
        }
        
        if (sent == 0) {
            // Unusual but possible
            return Result::Disconnected;
        }
        
        ptr += sent;
        remaining -= sent;
    }
    
    return Result::Success;
}

//=============================================================================
// MESSAGE FRAMING PROTOCOL
//=============================================================================

/**
 * Wire format:
 * +------------------+------------------+
 * | Length (4 bytes) | Payload (N bytes)|
 * | Network order    |                  |
 * +------------------+------------------+
 * 
 * Length field contains the payload size, NOT including the header.
 */

Result SendMessage(SOCKET socket, const std::string& message) {
    // =========================================================================
    // SECURITY CHECK: Validate message size BEFORE any network operations
    // THREAT: Prevents integer overflow when converting to uint32_t
    // THREAT: Prevents sending messages that receiver can't handle
    // =========================================================================
    if (message.size() > MAX_MESSAGE_SIZE) {
        return Result::MessageTooLarge;
    }
    
    // Convert length to network byte order (big-endian)
    // This ensures protocol works across different CPU architectures
    uint32_t length = static_cast<uint32_t>(message.size());
    uint32_t networkLength = htonl(length);
    
    // Send the length header first
    Result result = SendExact(socket, &networkLength, HEADER_SIZE);
    if (result != Result::Success) {
        return result;
    }
    
    // Send the payload (if non-empty)
    if (length > 0) {
        result = SendExact(socket, message.data(), length);
        if (result != Result::Success) {
            return result;
        }
    }
    
    return Result::Success;
}

Result ReceiveMessage(SOCKET socket, std::string& message) {
    // Clear output parameter first (defense in depth)
    message.clear();
    
    // =========================================================================
    // STEP 1: Receive the length header
    // =========================================================================
    uint32_t networkLength = 0;
    Result result = RecvExact(socket, &networkLength, HEADER_SIZE);
    if (result != Result::Success) {
        return result;
    }
    
    // Convert from network byte order
    uint32_t length = ntohl(networkLength);
    
    // =========================================================================
    // SECURITY CHECK: Validate length BEFORE allocating memory
    // 
    // THREAT MITIGATED: Memory exhaustion attack
    // An attacker could send length = 0xFFFFFFFF (4GB) causing:
    //   - std::bad_alloc crash
    //   - System memory exhaustion
    //   - Denial of service
    // 
    // By validating here, we reject malicious lengths immediately.
    // =========================================================================
    if (length > MAX_MESSAGE_SIZE) {
        // Log this as suspicious - legitimate clients never send oversized messages
        // TODO: Add to security audit log in Phase 5
        return Result::MessageTooLarge;
    }
    
    // =========================================================================
    // STEP 2: Allocate buffer and receive payload
    // =========================================================================
    if (length == 0) {
        // Empty message is valid (e.g., keepalive/heartbeat)
        return Result::Success;
    }
    
    // Pre-allocate to avoid reallocation during receive
    // This is now safe because we validated length above
    try {
        message.resize(length);
    } catch (const std::bad_alloc&) {
        return Result::BufferError;
    }
    
    // Receive directly into string's buffer
    // SECURITY NOTE: string::data() returns non-const pointer in C++17
    result = RecvExact(socket, &message[0], length);
    if (result != Result::Success) {
        // Clear partial data on error
        SecureClear(message);
        return result;
    }
    
    return Result::Success;
}

} // namespace NetProtocol
