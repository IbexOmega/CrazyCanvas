#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    enum ETextureFlags : uint32 
    {
        TEXTURE_FLAG_NONE               = 0,
        TEXTURE_FLAG_RENDER_TARGET      = BIT(1),
        TEXTURE_FLAG_SHADER_RESOURCE    = BIT(2),
        TEXTURE_FLAG_UNORDERED_ACCESS   = BIT(3),
        TEXTURE_FLAG_DEPTH_STENCIL      = BIT(4),
        TEXTURE_FLAG_COPY_SRC           = BIT(5),
        TEXTURE_FLAG_COPY_DST           = BIT(6),
    };

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
        EMemoryType     MemoryType      = EMemoryType::NONE;
        EFormat         Format          = EFormat::NONE;
        ETextureType    Type            = ETextureType::TEXTURE_NONE;
        uint32          Flags           = ETextureFlags::TEXTURE_FLAG_NONE;
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
        DECL_INTERFACE(ITexture);

        virtual TextureDesc GetDesc() const = 0;
    };
}
