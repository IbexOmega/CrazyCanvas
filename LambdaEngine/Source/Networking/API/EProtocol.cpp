#include "Networking/API/EProtocol.h"

namespace LambdaEngine
{
    String EProtocolParser::ToString(EProtocol protocol)
    {
        return protocol == EProtocol::UDP ? "UDP" : "TCP";
    }

    EProtocol EProtocolParser::FromString(const String& string)
    {
        return string == "UDP" ? EProtocol::UDP : EProtocol::TCP;
    }
}