# Security Documentation

## Project Overview

This document tracks the security evolution of the Winsock TCP Chat Application from an insecure baseline to a hardened, defensible architecture.

**Target Audience**: Final-year CS students, security auditors, MSc portfolio reviewers

---

## Phase 1: Network Correctness & Safety

**Status**: ✅ Implemented

### Threat Model

Before Phase 1, the application was vulnerable to:

| Threat | Severity | Attack Vector |
|--------|----------|---------------|
| Message Boundary Confusion | Critical | TCP stream fragmentation |
| Buffer Overflow | Critical | Oversized messages |
| Memory Exhaustion DoS | High | Malicious length headers |
| Log Injection | Medium | Control characters in usernames |
| Identity Spoofing | Medium | Embedded sender names in messages |

### Implementation Summary

#### 1. Length-Prefixed Message Framing (`NetProtocol.h/cpp`)

**Problem Solved**: TCP is a stream protocol. A single `recv()` call may return:
- Part of a message (network fragmentation)
- Multiple messages concatenated (Nagle's algorithm)
- Exactly one message (luck)

**Solution**: Wire format with explicit boundaries:

```
+------------------+------------------+
| Length (4 bytes) | Payload (N bytes)|
| Network order    |                  |
+------------------+------------------+
```

**Key Functions**:
- `SendMessage()` - Validates size, sends length prefix + payload
- `ReceiveMessage()` - Validates length before allocation, handles partial reads
- `RecvExact()` - Loops until exactly N bytes received
- `SendExact()` - Loops until all bytes sent

#### 2. Maximum Message Size Enforcement

```cpp
constexpr uint32_t MAX_MESSAGE_SIZE = 65536;  // 64 KB
```

**Threat Mitigated**: An attacker sending `length = 0xFFFFFFFF` (4GB) would cause:
- `std::bad_alloc` crash
- System memory exhaustion
- Denial of service

**Defense**: Length is validated BEFORE memory allocation:

```cpp
if (length > MAX_MESSAGE_SIZE) {
    return Result::MessageTooLarge;  // Reject immediately
}
```

#### 3. Socket Timeout Configuration

```cpp
constexpr int RECV_TIMEOUT_MS = 30000;  // 30 seconds
```

**Threat Mitigated**: Slowloris-style attacks where an attacker:
- Opens many connections
- Sends data very slowly
- Exhausts server connection slots

**Defense**: Sockets configured with `SO_RCVTIMEO` and `SO_SNDTIMEO`.

#### 4. Username Validation

**Validation Rules**:
- Length: 1-64 characters
- Character set: Printable ASCII only (0x20-0x7E)
- No leading/trailing whitespace
- No reserved prefixes (`[SERVER]`, `[SYSTEM]`, `[ADMIN]`)

**Threats Mitigated**:
- **Log Injection**: Newlines in username could forge log entries
- **Display Confusion**: Unicode homographs could impersonate users
- **Server Impersonation**: `[SERVER]` prefix could spoof system messages

#### 5. Secure Memory Clearing

```cpp
void SecureClear(void* buffer, size_t length);
```

**Purpose**: Prevents sensitive data from lingering in memory where it could be recovered via:
- Memory dumps
- Core dumps
- Cold boot attacks (in extreme cases)

**Implementation**: Uses `volatile` to prevent compiler optimization.

### Attacks Now Prevented

| Attack | Status | Defense |
|--------|--------|---------|
| TCP stream injection | ✅ Blocked | Length-prefixed framing |
| Buffer overflow via large message | ✅ Blocked | MAX_MESSAGE_SIZE validation |
| Memory exhaustion (fake length) | ✅ Blocked | Pre-allocation length check |
| Slowloris connection exhaustion | ✅ Mitigated | Socket timeouts |
| Username log injection | ✅ Blocked | ASCII-only validation |
| Server message spoofing | ✅ Blocked | Reserved prefix rejection |

### Known Remaining Weaknesses

These will be addressed in subsequent phases:

| Weakness | Severity | Phase to Address |
|----------|----------|------------------|
| No authentication | Critical | Phase 2 |
| Plaintext transmission | Critical | Phase 3 |
| No session management | High | Phase 4 |
| No replay protection | High | Phase 4 |
| No rate limiting | Medium | Phase 4 |
| Limited logging | Medium | Phase 5 |

### Code Audit Checklist

When reviewing Phase 1 code, verify:

- [ ] All `recv()` calls use `RecvExact()` or `ReceiveMessage()`
- [ ] All `send()` calls use `SendExact()` or `SendMessage()`
- [ ] Length is validated BEFORE `resize()` or allocation
- [ ] Timeouts are configured on all client sockets
- [ ] Usernames are validated before acceptance
- [ ] Server formats all broadcast messages (not clients)
- [ ] Error paths clear sensitive data

### Trust Boundaries

```
┌─────────────────────────────────────────────────────────────┐
│                        SERVER                                │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              TRUSTED ZONE                            │    │
│  │  - Server logic                                      │    │
│  │  - Validated usernames                               │    │
│  │  - Authenticated sessions (Phase 2+)                 │    │
│  └─────────────────────────────────────────────────────┘    │
│                           │                                  │
│                    TRUST BOUNDARY                            │
│                           │                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           UNTRUSTED ZONE                             │    │
│  │  - All data from recv()                              │    │
│  │  - Message content                                   │    │
│  │  - Length headers                                    │    │
│  │  - Client-claimed identities                         │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                           │
                      NETWORK
                           │
┌─────────────────────────────────────────────────────────────┐
│                    ATTACKER-CONTROLLED                       │
│  - Network traffic                                           │
│  - Packet timing                                             │
│  - Connection patterns                                       │
└─────────────────────────────────────────────────────────────┘
```

### Testing Recommendations

To verify Phase 1 security:

1. **Fragmentation Test**: Send a message in 1-byte chunks
2. **Oversized Length Test**: Send length header of 0xFFFFFFFF
3. **Slow Client Test**: Send 1 byte per second, verify timeout
4. **Username Injection Test**: Try usernames with `\n`, `\r`, `\x00`
5. **Reserved Prefix Test**: Try username `[SERVER]Admin`

---

## Phase 2: Authentication & Identity

**Status**: ✅ Implemented

### Threat Model

Before Phase 2, the application was vulnerable to:

| Threat | Severity | Attack Vector |
|--------|----------|---------------|
| Identity Spoofing | Critical | Anyone can claim any username |
| Unauthorized Access | Critical | No credential verification |
| Password Theft | Critical | Plaintext storage (if implemented naively) |
| Session Hijacking | High | No session tokens |
| Timing Attacks | Medium | Non-constant password comparison |
| Credential Stuffing | Medium | No rate limiting on login |

### Implementation Summary

#### 1. Data Models (`Models.h/cpp`)

Comprehensive data structures for Discord-like functionality:

- **User**: Unique ID, username, online status, server memberships, friends
- **ChatServer**: Server with channels, owner, members
- **Channel**: Text channel within a server
- **Message**: Chat message with sender, channel, timestamp
- **FriendRequest**: Friend request with status tracking
- **Session**: Authenticated session with token and expiry

**Security Features**:
- All entities use 64-bit unique IDs (timestamp + random component)
- Maximum limits on all collections to prevent resource exhaustion
- Input validation on all string fields

```cpp
constexpr size_t MAX_USERNAME_LENGTH = 32;
constexpr size_t MAX_SERVER_NAME_LENGTH = 64;
constexpr size_t MIN_PASSWORD_LENGTH = 8;
constexpr size_t MAX_SERVERS_PER_USER = 100;
constexpr size_t MAX_FRIENDS_PER_USER = 1000;
```

#### 2. Protocol Definition (`Protocol.h/cpp`)

Application-level protocol for structured client-server communication:

**Request Types**: 28+ defined operations (Register, Login, CreateServer, SendMessage, etc.)

**Response Types**: Corresponding server responses and events

**Error Codes**: 25+ specific error codes for precise error handling

**Security Benefits**:
- Type-safe message handling
- No string-based command parsing vulnerabilities
- Explicit error codes prevent information leakage

#### 3. User Database (`UserDatabase.h/cpp`)

Secure credential storage and authentication system:

**Password Hashing**:
```cpp
// SHA-256 with unique per-user salt
std::string HashPassword(const std::string& password, const std::string& salt);

// Salt: 32 bytes of cryptographically secure random
std::string GenerateSalt();
```

**Constant-Time Comparison**:
```cpp
bool ConstantTimeCompare(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    volatile unsigned char result = 0;
    for (size_t i = 0; i < a.length(); i++) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}
```

**Secure Random Generation** (Windows Crypto API):
```cpp
// Uses CryptGenRandom for cryptographically secure randomness
// Fallback to std::random_device if unavailable
std::string GenerateSecureRandom(size_t length);
```

**Session Management**:
- 64-byte session tokens (128 hex characters)
- 24-hour session expiry
- Activity-based session refresh
- Explicit session invalidation on logout

**Memory Security**:
- Immediate clearing of password strings after hashing
- Uses volatile pointers to prevent optimization
- Password hashes stored separately from user data

#### 4. Server Manager (`ServerManager.h/cpp`)

Secure server and channel management:

**Authorization Checks**:
- Only owners can rename/delete servers
- Only owners can create/delete channels
- Members automatically removed when server deleted
- Ownership transfer on owner departure

**Resource Limits**:
- Max servers per user: 100
- Max channels per server: 50
- Max members per server: 10,000

#### 5. Friend Service (`FriendService.h/cpp`)

Secure friend relationship management:

**Security Features**:
- Cannot friend yourself
- Duplicate request prevention
- Mutual friend requests auto-accept
- Request cancellation by sender only
- Request acceptance/decline by receiver only

### Attacks Now Prevented

| Attack | Status | Defense |
|--------|--------|---------|
| Identity Spoofing | ✅ Blocked | Session-based authentication |
| Password Guessing | ✅ Mitigated | Salted hashing, min length |
| Rainbow Table Attack | ✅ Blocked | Per-user salt |
| Timing Attack on Login | ✅ Blocked | Constant-time comparison |
| Credential Harvesting | ✅ Blocked | Hash stored, not plaintext |
| Session Fixation | ✅ Blocked | Server-generated tokens |
| Unauthorized Server Access | ✅ Blocked | Membership verification |

### Known Remaining Weaknesses

| Weakness | Severity | Phase to Address |
|----------|----------|------------------|
| SHA-256 not ideal for passwords | Medium | Consider Argon2/bcrypt |
| Plaintext network transmission | Critical | Phase 3 |
| No login rate limiting | Medium | Phase 4 |
| No account lockout | Medium | Phase 4 |
| Session token in memory | Low | Hardware security module |

### Security Recommendations

1. **Production Password Hashing**: Replace SHA-256 with Argon2id:
   - Memory-hard (resists GPU attacks)
   - Time-hard (configurable iterations)
   - Salt integrated into algorithm

2. **Multi-Factor Authentication**: Add TOTP support in future phase

3. **Password Policy**: Enforce complexity requirements:
   - Mix of character types
   - No common passwords (dictionary check)

### Trust Boundaries (Updated)

```
┌─────────────────────────────────────────────────────────────┐
│                        SERVER                                │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              FULLY TRUSTED                          │    │
│  │  - UserDatabase (hashed passwords)                  │    │
│  │  - ServerManager (validated data)                   │    │
│  │  - Session store                                    │    │
│  └─────────────────────────────────────────────────────┘    │
│                           │                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │        AUTHENTICATED (PARTIALLY TRUSTED)            │    │
│  │  - Valid session token                              │    │
│  │  - User ID verified                                 │    │
│  │  - Still validate all actions                       │    │
│  └─────────────────────────────────────────────────────┘    │
│                           │                                  │
│                    TRUST BOUNDARY                            │
│                           │                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           UNTRUSTED (ALL NETWORK INPUT)             │    │
│  │  - Login credentials (could be attacker)            │    │
│  │  - Session tokens (could be stolen)                 │    │
│  │  - All request payloads                             │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

---

## Future Phases

### Phase 3: Confidentiality & Integrity
- TLS encryption (SChannel or OpenSSL)
- Certificate validation
- Message integrity (HMAC or TLS guarantees)

### Phase 4: Session & Protocol Hardening
- Replay protection (nonces/sequence numbers)
- Rate limiting on authentication
- Account lockout after failed attempts

### Phase 5: Attack Awareness
- Structured security logging
- Protocol violation detection
- Explicit trust boundary comments

---

## Changelog

### v0.2.0 - Phase 2 (Current)
- Added `Models.h/cpp` - Data models with validation
- Added `Protocol.h/cpp` - Application-level protocol
- Added `UserDatabase.h/cpp` - Secure authentication
- Added `ServerManager.h/cpp` - Server/channel management
- Added `FriendService.h/cpp` - Friend request system
- Added `LoginPage.h/cpp` - Authentication UI
- Added `ServerBrowser.h/cpp` - Server list UI
- Added `ChannelList.h/cpp` - Channel navigation UI
- Implemented SHA-256 password hashing with salt
- Implemented constant-time password comparison
- Implemented cryptographically secure session tokens
- Added session expiry and activity tracking

### v0.1.0 - Phase 1
- Added `NetProtocol.h/cpp` for secure message framing
- Implemented length-prefixed message protocol
- Added username validation
- Configured socket timeouts
- Added secure memory clearing utilities
- Updated `ClientSocket` with `sendSecure()`/`receiveSecure()`
- Updated `ServerSocket` with `broadcastMessageSecure()`

---

## Contact

For security concerns or vulnerability reports, contact the project maintainer.
