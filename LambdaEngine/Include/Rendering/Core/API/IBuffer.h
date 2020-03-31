#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    enum EBufferFlags
    {
        BUFFER_FLAG_NONE                    = 0,
        BUFFER_FLAG_VERTEX_BUFFER           = BIT(1),
        BUFFER_FLAG_INDEX_BUFFER            = BIT(2),
        BUFFER_FLAG_UNORDERED_ACCESS_BUFFER = BIT(3),
        BUFFER_FLAG_CONSTANT_BUFFER         = BIT(4),
        BUFFER_FLAG_COPY_DST                = BIT(5),
        BUFFER_FLAG_COPY_SRC                = BIT(5),
    };

    struct BufferDesc
    {
        const char* pName       = "";
        EMemoryType MemoryType  = EMemoryType::NONE;
        uint32      Flags       = EBufferFlags::BUFFER_FLAG_NONE;
        uint32      SizeInBytes = 0;
    };

    class IBuffer : public IDeviceChild
    {
    public:
        DECL_INTERFACE(IBuffer);

        virtual void*   Map()   = 0;
        virtual void    Unmap() = 0;

        virtual BufferDesc GetDesc() const = 0;
    };
}
