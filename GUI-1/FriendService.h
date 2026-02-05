#ifndef FRIEND_SERVICE_H
#define FRIEND_SERVICE_H

/**
 * @file FriendService.h
 * @brief Manages friend relationships and friend requests
 * 
 * ARCHITECTURE:
 * - Users can send friend requests to other users
 * - Requests can be accepted or declined
 * - Mutual friendship (both users must agree)
 * - Limits on max friends to prevent abuse
 * 
 * SECURITY:
 * - Cannot friend yourself
 * - Cannot send duplicate requests
 * - Request spam prevention (limits)
 * - Proper validation of all user IDs
 */

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "Models.h"
#include "Protocol.h"
#include "pugixml.hpp"

// Forward declaration
class UserDatabase;

class FriendService {
public:
    /**
     * @brief Initialize with database path and user database reference
     * @param databasePath Path to XML storage file
     * @param userDb Reference to user database
     */
    FriendService(const std::string& databasePath, UserDatabase& userDb);
    
    ~FriendService();
    
    // =========================================================================
    // FRIEND REQUEST OPERATIONS
    // =========================================================================
    
    /**
     * @brief Send a friend request to another user
     * 
     * @param senderId User sending the request
     * @param receiverId User receiving the request
     * @param outRequest Output: created request
     * @return Error code (None on success)
     * 
     * Possible errors:
     * - CannotFriendSelf: Tried to friend yourself
     * - UserNotFound: Receiver doesn't exist
     * - AlreadyFriends: Already friends with this user
     * - RequestAlreadySent: Pending request already exists
     * - TooManyFriends: Sender at friend limit
     */
    Protocol::ErrorCode SendFriendRequest(
        uint64_t senderId,
        uint64_t receiverId,
        Models::FriendRequest& outRequest
    );
    
    /**
     * @brief Accept a pending friend request
     * 
     * @param requestId Request to accept
     * @param accepterId User accepting (must be receiver)
     * @return Error code (None on success)
     * 
     * Possible errors:
     * - RequestNotFound: Request doesn't exist or already processed
     * - NotAuthorized: User is not the receiver of this request
     * - TooManyFriends: Either user is at friend limit
     */
    Protocol::ErrorCode AcceptFriendRequest(uint64_t requestId, uint64_t accepterId);
    
    /**
     * @brief Decline a pending friend request
     * 
     * @param requestId Request to decline
     * @param declinerId User declining (must be receiver)
     * @return Error code (None on success)
     */
    Protocol::ErrorCode DeclineFriendRequest(uint64_t requestId, uint64_t declinerId);
    
    /**
     * @brief Cancel a sent friend request
     * 
     * @param requestId Request to cancel
     * @param cancelerId User canceling (must be sender)
     * @return Error code (None on success)
     */
    Protocol::ErrorCode CancelFriendRequest(uint64_t requestId, uint64_t cancelerId);
    
    /**
     * @brief Get all pending friend requests for a user
     * 
     * @param userId User to get requests for
     * @param incoming True to get incoming requests, false for outgoing
     * @return Vector of pending requests
     */
    std::vector<Models::FriendRequest> GetPendingRequests(uint64_t userId, bool incoming);
    
    /**
     * @brief Get a specific friend request by ID
     * 
     * @param requestId Request ID
     * @param outRequest Output: request data
     * @return True if found
     */
    bool GetFriendRequest(uint64_t requestId, Models::FriendRequest& outRequest);
    
    // =========================================================================
    // FRIENDSHIP OPERATIONS
    // =========================================================================
    
    /**
     * @brief Remove a friend
     * 
     * @param userId User removing the friend
     * @param friendId Friend to remove
     * @return Error code (None on success)
     * 
     * Note: This removes the friendship for both users
     */
    Protocol::ErrorCode RemoveFriend(uint64_t userId, uint64_t friendId);
    
    /**
     * @brief Check if two users are friends
     */
    bool AreFriends(uint64_t userId1, uint64_t userId2);
    
    /**
     * @brief Get a user's friend list
     * 
     * @param userId User to get friends for
     * @return Vector of friend user IDs
     */
    std::vector<uint64_t> GetFriendIds(uint64_t userId);
    
    /**
     * @brief Get count of pending incoming requests for a user
     * 
     * Useful for displaying notification badges
     */
    size_t GetPendingRequestCount(uint64_t userId);
    
    // =========================================================================
    // PERSISTENCE
    // =========================================================================
    
    bool SaveToFile();
    bool LoadFromFile();
    
private:
    std::string databaseFilePath;
    UserDatabase& userDatabase;
    
    // In-memory storage
    std::map<uint64_t, Models::FriendRequest> requestsById;
    
    // Thread safety
    mutable std::mutex serviceMutex;
    
    // Helper to check for existing pending request
    bool HasPendingRequest(uint64_t senderId, uint64_t receiverId);
    
    // Helper to get request between two users
    bool GetPendingRequestBetween(
        uint64_t user1Id, 
        uint64_t user2Id, 
        Models::FriendRequest& outRequest
    );
};

#endif // FRIEND_SERVICE_H
