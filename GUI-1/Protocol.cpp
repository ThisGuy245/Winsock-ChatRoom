/**
 * @file Protocol.cpp
 * @brief Implementation of protocol helper functions
 */

#include "Protocol.h"
#include <atomic>

namespace Protocol {

//=============================================================================
// STRING CONVERSION HELPERS
//=============================================================================

const char* RequestTypeToString(RequestType type) {
    switch (type) {
        // Authentication
        case RequestType::Register:           return "Register";
        case RequestType::Login:              return "Login";
        case RequestType::Logout:             return "Logout";
        
        // Server Management
        case RequestType::CreateServer:       return "CreateServer";
        case RequestType::DeleteServer:       return "DeleteServer";
        case RequestType::JoinServer:         return "JoinServer";
        case RequestType::LeaveServer:        return "LeaveServer";
        case RequestType::RenameServer:       return "RenameServer";
        case RequestType::GetServerList:      return "GetServerList";
        case RequestType::GetServerMembers:   return "GetServerMembers";
        
        // Channel Management
        case RequestType::CreateChannel:      return "CreateChannel";
        case RequestType::DeleteChannel:      return "DeleteChannel";
        case RequestType::RenameChannel:      return "RenameChannel";
        case RequestType::GetChannelList:     return "GetChannelList";
        case RequestType::JoinChannel:        return "JoinChannel";
        case RequestType::LeaveChannel:       return "LeaveChannel";
        
        // Messaging
        case RequestType::SendMessage:        return "SendMessage";
        case RequestType::SendDirectMessage:  return "SendDirectMessage";
        case RequestType::GetMessageHistory:  return "GetMessageHistory";
        
        // Friends
        case RequestType::SendFriendRequest:  return "SendFriendRequest";
        case RequestType::AcceptFriendRequest:return "AcceptFriendRequest";
        case RequestType::DeclineFriendRequest:return "DeclineFriendRequest";
        case RequestType::RemoveFriend:       return "RemoveFriend";
        case RequestType::GetFriendList:      return "GetFriendList";
        case RequestType::GetFriendRequests:  return "GetFriendRequests";
        
        // User Info
        case RequestType::GetUserProfile:     return "GetUserProfile";
        case RequestType::UpdateProfile:      return "UpdateProfile";
        case RequestType::SearchUsers:        return "SearchUsers";
        
        // Presence
        case RequestType::Heartbeat:          return "Heartbeat";
        
        default:                              return "Unknown";
    }
}

const char* ResponseTypeToString(ResponseType type) {
    switch (type) {
        // Generic
        case ResponseType::Success:           return "Success";
        case ResponseType::Error:             return "Error";
        
        // Authentication
        case ResponseType::LoginSuccess:      return "LoginSuccess";
        case ResponseType::RegisterSuccess:   return "RegisterSuccess";
        case ResponseType::SessionExpired:    return "SessionExpired";
        
        // Data Responses
        case ResponseType::ServerList:        return "ServerList";
        case ResponseType::ChannelList:       return "ChannelList";
        case ResponseType::MemberList:        return "MemberList";
        case ResponseType::MessageHistory:    return "MessageHistory";
        case ResponseType::FriendList:        return "FriendList";
        case ResponseType::FriendRequestList: return "FriendRequestList";
        case ResponseType::UserProfile:       return "UserProfile";
        case ResponseType::SearchResults:     return "SearchResults";
        
        // Real-time Events
        case ResponseType::NewMessage:        return "NewMessage";
        case ResponseType::NewDirectMessage:  return "NewDirectMessage";
        case ResponseType::UserJoined:        return "UserJoined";
        case ResponseType::UserLeft:          return "UserLeft";
        case ResponseType::UserOnline:        return "UserOnline";
        case ResponseType::UserOffline:       return "UserOffline";
        case ResponseType::FriendRequestReceived: return "FriendRequestReceived";
        case ResponseType::ServerUpdated:     return "ServerUpdated";
        case ResponseType::ChannelUpdated:    return "ChannelUpdated";
        case ResponseType::ServerDeleted:     return "ServerDeleted";
        case ResponseType::ChannelDeleted:    return "ChannelDeleted";
        case ResponseType::Kicked:            return "Kicked";
        
        default:                              return "Unknown";
    }
}

const char* ErrorCodeToMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::None:                 return "No error";
        
        // Authentication errors
        case ErrorCode::InvalidCredentials:   return "Invalid username or password";
        case ErrorCode::UsernameAlreadyExists:return "Username is already taken";
        case ErrorCode::InvalidUsername:      return "Invalid username format";
        case ErrorCode::InvalidPassword:      return "Password does not meet requirements";
        case ErrorCode::SessionExpired:       return "Your session has expired, please login again";
        case ErrorCode::NotAuthenticated:     return "You must be logged in to do that";
        
        // Permission errors
        case ErrorCode::NotAuthorized:        return "You don't have permission to do that";
        case ErrorCode::NotServerOwner:       return "Only the server owner can do that";
        case ErrorCode::NotServerMember:      return "You are not a member of this server";
        
        // Resource errors
        case ErrorCode::ServerNotFound:       return "Server not found";
        case ErrorCode::ChannelNotFound:      return "Channel not found";
        case ErrorCode::UserNotFound:         return "User not found";
        case ErrorCode::MessageNotFound:      return "Message not found";
        
        // Validation errors
        case ErrorCode::InvalidServerName:    return "Invalid server name";
        case ErrorCode::InvalidChannelName:   return "Invalid channel name (use lowercase letters, numbers, and hyphens)";
        case ErrorCode::InvalidMessageContent:return "Invalid message content";
        case ErrorCode::TooManyServers:       return "You have reached the maximum number of servers";
        case ErrorCode::TooManyChannels:      return "This server has reached the maximum number of channels";
        case ErrorCode::TooManyFriends:       return "You have reached the maximum number of friends";
        
        // Friend errors
        case ErrorCode::AlreadyFriends:       return "You are already friends with this user";
        case ErrorCode::RequestAlreadySent:   return "You already sent a friend request to this user";
        case ErrorCode::RequestNotFound:      return "Friend request not found";
        case ErrorCode::CannotFriendSelf:     return "You cannot add yourself as a friend";
        
        // Server errors
        case ErrorCode::InternalError:        return "An internal error occurred";
        case ErrorCode::RateLimited:          return "You are sending requests too quickly";
        case ErrorCode::ServerOverloaded:     return "Server is currently overloaded, please try again";
        
        default:                              return "Unknown error";
    }
}

uint32_t GenerateRequestId() {
    static std::atomic<uint32_t> counter(1);
    return counter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace Protocol
