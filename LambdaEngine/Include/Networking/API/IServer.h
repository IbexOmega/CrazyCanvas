#pragma once

#include "LambdaEngine.h"
#include "Containers/String.h"
#include "Containers/TSet.h"

namespace LambdaEngine
{
    class LAMBDA_API IServer
    {
        class IPAddress;

    public:
        DECL_INTERFACE(IServer);

        virtual const std::set<IPAddress*>& GetBannedAddresses() = 0;
        //virtual void DisconnectPlayer(NetworkingPlayer player, bool forced);
        virtual void BanPlayer(uint64 networkId) = 0;
        virtual void CommitDisconnects() = 0;
        virtual void SetAcceptingConnections(bool accepting) = 0;
        virtual bool IsAcceptingConnections() = 0;
    };
}