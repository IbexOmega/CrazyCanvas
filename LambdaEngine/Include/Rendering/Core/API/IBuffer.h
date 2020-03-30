#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    enum EBufferFlags
    {
        BUFFER_FLAG_NONE = 0,
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
        virtual void    UnMap() = 0;

        virtual BufferDesc GetDesc() const = 0;
    };
}