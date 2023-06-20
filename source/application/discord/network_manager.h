#pragma once

#include "types.h"

namespace discord {

class NetworkManager final {
public:
    ~NetworkManager() = default;

    void GetPeerId(NetworkPeerId* peerId);
    Result Flush();
    Result OpenPeer(NetworkPeerId peerId, char const* routeData);
    Result UpdatePeer(NetworkPeerId peerId, char const* routeData);
    Result ClosePeer(NetworkPeerId peerId);
    Result OpenChannel(NetworkPeerId peerId, NetworkChannelId channelId, bool reliable);
    Result CloseChannel(NetworkPeerId peerId, NetworkChannelId channelId);
    Result SendMessage(NetworkPeerId peerId,
                       NetworkChannelId channelId,
                       std::uint8_t* data,
                       std::uint32_t dataLength);

    Event<NetworkPeerId, NetworkChannelId, std::uint8_t*, std::uint32_t> OnMessage;
    Event<char const*> OnRouteUpdate;

private:
    friend class Core;

    NetworkManager() = default;
    NetworkManager(NetworkManager const& rhs) = delete;
    NetworkManager& operator=(NetworkManager const& rhs) = delete;
    NetworkManager(NetworkManager&& rhs) = delete;
    NetworkManager& operator=(NetworkManager&& rhs) = delete;

    IDiscordNetworkManager* internal_;
    static IDiscordNetworkEvents events_;
};

} // namespace discord
