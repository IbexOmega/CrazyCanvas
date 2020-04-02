#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    enum EBufferFlags
    {
        BUFFER_FLAG_NONE                    = 0,
        BUFFER_FLAG_VERTEX_BUFFER           = FLAG(1),
        BUFFER_FLAG_INDEX_BUFFER            = FLAG(2),
        BUFFER_FLAG_UNORDERED_ACCESS_BUFFER = FLAG(3),
        BUFFER_FLAG_CONSTANT_BUFFER         = FLAG(4),
        BUFFER_FLAG_COPY_DST                = FLAG(5),
        BUFFER_FLAG_COPY_SRC                = FLAG(5),
		BUFFER_FLAG_RAY_TRACING				= FLAG(6),
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

        virtual uint64      GetHandle()         const = 0;
        virtual uint64      GetDeviceAdress()   const = 0;
        virtual BufferDesc  GetDesc()           const = 0;
    };
}
