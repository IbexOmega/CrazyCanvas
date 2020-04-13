#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
    class NetworkPacket;

    class LAMBDA_API IDispatcherHandler
    {
    public:
        DECL_INTERFACE(IDispatcherHandler);

        virtual void OnPacketReceived(NetworkPacket* packet) = 0;
    };
}