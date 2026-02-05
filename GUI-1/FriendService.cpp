/**
 * @file FriendService.cpp
 * @brief Implementation of friend request and friendship management
 */

#include "FriendService.h"
#include "UserDatabase.h"
#include <algorithm>

//=============================================================================
// CONSTRUCTOR / DESTRUCTOR
//=============================================================================

FriendService::FriendService(const std::string& databasePath, UserDatabase& userDb)
    : databaseFilePath(databasePath)
    , userDatabase(userDb) {
    LoadFromFile();
}

FriendService::~FriendService() {
    SaveToFile();
}

//=============================================================================
// FRIEND REQUEST OPERATIONS
//=============================================================================

Protocol::ErrorCode FriendService::SendFriendRequest(
    uint64_t senderId,
    uint64_t receiverId,
    Models::FriendRequest& outRequest
) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    // Cannot friend yourself
    if (senderId == receiverId) {
        return Protocol::ErrorCode::CannotFriendSelf;
    }
    
    // Check receiver exists
    Models::User receiver;
    if (!userDatabase.GetUserById(receiverId, receiver)) {
        return Protocol::ErrorCode::UserNotFound;
    }
    
    // Check sender exists
    Models::User sender;
    if (!userDatabase.GetUserById(senderId, sender)) {
        return Protocol::ErrorCode::UserNotFound;
    }
    
    // Check if already friends
    if (userDatabase.AreFriends(senderId, receiverId)) {
        return Protocol::ErrorCode::AlreadyFriends;
    }
    
    // Check for existing pending request (either direction)
    if (HasPendingRequest(senderId, receiverId)) {
        return Protocol::ErrorCode::RequestAlreadySent;
    }
    
    // Check if there's an incoming request from receiver to sender
    // If so, auto-accept it instead of creating a new one
    Models::FriendRequest existingRequest;
    if (GetPendingRequestBetween(receiverId, senderId, existingRequest)) {
        // The receiver already sent us a request, auto-accept
        existingRequest.status = Models::FriendRequest::Status::Accepted;
        requestsById[existingRequest.requestId] = existingRequest;
        
        // Create friendship
        userDatabase.AddFriendship(senderId, receiverId);
        
        outRequest = existingRequest;
        
        printf("[FRIEND] Auto-accepted mutual friend request between %llu and %llu\n",
               senderId, receiverId);
        
        SaveToFile();
        return Protocol::ErrorCode::None;
    }
    
    // Check friend limit for sender
    std::vector<Models::User> senderFriends = userDatabase.GetFriends(senderId);
    if (senderFriends.size() >= Models::MAX_FRIENDS_PER_USER) {
        return Protocol::ErrorCode::TooManyFriends;
    }
    
    // Create the request
    uint64_t requestId = Models::GenerateUniqueId();
    Models::FriendRequest request(requestId, senderId, receiverId);
    
    requestsById[requestId] = request;
    outRequest = request;
    
    SaveToFile();
    
    printf("[FRIEND] User %llu sent friend request to user %llu\n", senderId, receiverId);
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode FriendService::AcceptFriendRequest(uint64_t requestId, uint64_t accepterId) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    auto it = requestsById.find(requestId);
    if (it == requestsById.end()) {
        return Protocol::ErrorCode::RequestNotFound;
    }
    
    Models::FriendRequest& request = it->second;
    
    // Must be pending
    if (request.status != Models::FriendRequest::Status::Pending) {
        return Protocol::ErrorCode::RequestNotFound;
    }
    
    // Only receiver can accept
    if (request.receiverId != accepterId) {
        return Protocol::ErrorCode::NotAuthorized;
    }
    
    // Check friend limits
    std::vector<Models::User> senderFriends = userDatabase.GetFriends(request.senderId);
    std::vector<Models::User> receiverFriends = userDatabase.GetFriends(request.receiverId);
    
    if (senderFriends.size() >= Models::MAX_FRIENDS_PER_USER ||
        receiverFriends.size() >= Models::MAX_FRIENDS_PER_USER) {
        return Protocol::ErrorCode::TooManyFriends;
    }
    
    // Accept the request
    request.status = Models::FriendRequest::Status::Accepted;
    
    // Create friendship
    userDatabase.AddFriendship(request.senderId, request.receiverId);
    
    SaveToFile();
    
    printf("[FRIEND] User %llu accepted friend request from user %llu\n",
           accepterId, request.senderId);
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode FriendService::DeclineFriendRequest(uint64_t requestId, uint64_t declinerId) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    auto it = requestsById.find(requestId);
    if (it == requestsById.end()) {
        return Protocol::ErrorCode::RequestNotFound;
    }
    
    Models::FriendRequest& request = it->second;
    
    // Must be pending
    if (request.status != Models::FriendRequest::Status::Pending) {
        return Protocol::ErrorCode::RequestNotFound;
    }
    
    // Only receiver can decline
    if (request.receiverId != declinerId) {
        return Protocol::ErrorCode::NotAuthorized;
    }
    
    // Decline the request
    request.status = Models::FriendRequest::Status::Declined;
    
    SaveToFile();
    
    printf("[FRIEND] User %llu declined friend request from user %llu\n",
           declinerId, request.senderId);
    
    return Protocol::ErrorCode::None;
}

Protocol::ErrorCode FriendService::CancelFriendRequest(uint64_t requestId, uint64_t cancelerId) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    auto it = requestsById.find(requestId);
    if (it == requestsById.end()) {
        return Protocol::ErrorCode::RequestNotFound;
    }
    
    Models::FriendRequest& request = it->second;
    
    // Must be pending
    if (request.status != Models::FriendRequest::Status::Pending) {
        return Protocol::ErrorCode::RequestNotFound;
    }
    
    // Only sender can cancel
    if (request.senderId != cancelerId) {
        return Protocol::ErrorCode::NotAuthorized;
    }
    
    // Remove the request
    requestsById.erase(it);
    
    SaveToFile();
    
    printf("[FRIEND] User %llu cancelled friend request to user %llu\n",
           cancelerId, request.receiverId);
    
    return Protocol::ErrorCode::None;
}

std::vector<Models::FriendRequest> FriendService::GetPendingRequests(uint64_t userId, bool incoming) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    std::vector<Models::FriendRequest> results;
    
    for (const auto& pair : requestsById) {
        const Models::FriendRequest& request = pair.second;
        
        if (request.status != Models::FriendRequest::Status::Pending) {
            continue;
        }
        
        if (incoming && request.receiverId == userId) {
            results.push_back(request);
        } else if (!incoming && request.senderId == userId) {
            results.push_back(request);
        }
    }
    
    return results;
}

bool FriendService::GetFriendRequest(uint64_t requestId, Models::FriendRequest& outRequest) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    auto it = requestsById.find(requestId);
    if (it == requestsById.end()) {
        return false;
    }
    
    outRequest = it->second;
    return true;
}

//=============================================================================
// FRIENDSHIP OPERATIONS
//=============================================================================

Protocol::ErrorCode FriendService::RemoveFriend(uint64_t userId, uint64_t friendId) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    // Check they're actually friends
    if (!userDatabase.AreFriends(userId, friendId)) {
        return Protocol::ErrorCode::UserNotFound;  // Not friends
    }
    
    // Remove friendship
    userDatabase.RemoveFriendship(userId, friendId);
    
    printf("[FRIEND] User %llu removed friend %llu\n", userId, friendId);
    
    return Protocol::ErrorCode::None;
}

bool FriendService::AreFriends(uint64_t userId1, uint64_t userId2) {
    return userDatabase.AreFriends(userId1, userId2);
}

std::vector<uint64_t> FriendService::GetFriendIds(uint64_t userId) {
    std::vector<Models::User> friends = userDatabase.GetFriends(userId);
    std::vector<uint64_t> ids;
    ids.reserve(friends.size());
    
    for (const Models::User& friendUser : friends) {
        ids.push_back(friendUser.userId);
    }
    
    return ids;
}

size_t FriendService::GetPendingRequestCount(uint64_t userId) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    size_t count = 0;
    for (const auto& pair : requestsById) {
        if (pair.second.status == Models::FriendRequest::Status::Pending &&
            pair.second.receiverId == userId) {
            count++;
        }
    }
    
    return count;
}

//=============================================================================
// HELPER METHODS
//=============================================================================

bool FriendService::HasPendingRequest(uint64_t senderId, uint64_t receiverId) {
    for (const auto& pair : requestsById) {
        const Models::FriendRequest& request = pair.second;
        
        if (request.status != Models::FriendRequest::Status::Pending) {
            continue;
        }
        
        if (request.senderId == senderId && request.receiverId == receiverId) {
            return true;
        }
    }
    
    return false;
}

bool FriendService::GetPendingRequestBetween(
    uint64_t user1Id, 
    uint64_t user2Id, 
    Models::FriendRequest& outRequest
) {
    for (auto& pair : requestsById) {
        Models::FriendRequest& request = pair.second;
        
        if (request.status != Models::FriendRequest::Status::Pending) {
            continue;
        }
        
        if (request.senderId == user1Id && request.receiverId == user2Id) {
            outRequest = request;
            return true;
        }
    }
    
    return false;
}

//=============================================================================
// PERSISTENCE
//=============================================================================

bool FriendService::SaveToFile() {
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("FriendDatabase");
    
    pugi::xml_node requestsNode = root.append_child("Requests");
    for (const auto& pair : requestsById) {
        const Models::FriendRequest& request = pair.second;
        pugi::xml_node requestNode = requestsNode.append_child("Request");
        
        requestNode.append_attribute("id") = request.requestId;
        requestNode.append_attribute("senderId") = request.senderId;
        requestNode.append_attribute("receiverId") = request.receiverId;
        requestNode.append_attribute("status") = static_cast<int>(request.status);
        requestNode.append_attribute("createdAt") = static_cast<long long>(request.createdAt);
    }
    
    return doc.save_file(databaseFilePath.c_str());
}

bool FriendService::LoadFromFile() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(databaseFilePath.c_str());
    
    if (!result) {
        printf("[DB] No existing friend database found, starting fresh\n");
        return false;
    }
    
    pugi::xml_node root = doc.child("FriendDatabase");
    if (!root) return false;
    
    pugi::xml_node requestsNode = root.child("Requests");
    for (pugi::xml_node requestNode : requestsNode.children("Request")) {
        Models::FriendRequest request;
        request.requestId = requestNode.attribute("id").as_ullong();
        request.senderId = requestNode.attribute("senderId").as_ullong();
        request.receiverId = requestNode.attribute("receiverId").as_ullong();
        request.status = static_cast<Models::FriendRequest::Status>(
            requestNode.attribute("status").as_int()
        );
        request.createdAt = requestNode.attribute("createdAt").as_llong();
        
        requestsById[request.requestId] = request;
    }
    
    printf("[DB] Loaded %zu friend requests\n", requestsById.size());
    
    return true;
}
