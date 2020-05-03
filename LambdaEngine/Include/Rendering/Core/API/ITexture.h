#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    enum class ETextureType : uint8
    {
        TEXTURE_NONE    = 0,
        TEXTURE_1D      = 1,
        TEXTURE_2D      = 2,
        TEXTURE_3D      = 3,
    };

    struct TextureDesc
    {
        const char*     pName           = "";
        EMemoryType     MemoryType      = EMemoryType::MEMORY_TYPE_NONE;
        EFormat         Format          = EFormat::FORMAT_NONE;
        ETextureType    Type            = ETextureType::TEXTURE_NONE;
        uint32          Flags           = FTextureFlags::TEXTURE_FLAG_NONE;
        uint32          Width           = 0;
        uint32          Height          = 0;
        uint32          Depth           = 0;
        uint32          ArrayCount      = 0;
        uint32          Miplevels       = 0;
        uint32          SampleCount     = 0;
    };

    class ITexture : public IDeviceChild
    {
    public:
        DECL_DEVICE_INTERFACE(ITexture);

        /*
        * Returns the API-specific handle to the underlaying texture-resource
        *
        * return - Returns a valid handle on success otherwise zero
        */
        virtual uint64      GetHandle() const = 0;
        virtual TextureDesc GetDesc()   const = 0;
    };
}
