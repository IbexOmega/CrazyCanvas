#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IPEndPoint.h"

namespace LambdaEngine
{
    class LAMBDA_API IServer
    {
    public:
        DECL_INTERFACE(IServer);

        virtual bool Start(const IPEndPoint& ipEndPoint) = 0;
        virtual void Stop() = 0;
        virtual bool IsRunning() = 0;
        virtual const IPEndPoint& GetEndPoint() const = 0;
        virtual void SetAcceptingConnections(bool accepting) = 0;
        virtual bool IsAcceptingConnections() = 0;
    };
}