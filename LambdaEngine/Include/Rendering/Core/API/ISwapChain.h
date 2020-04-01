#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    class Window;
    class ITexture;
    
    struct SwapChainDesc
    {
        const char* pName           = "";
        EFormat     Format          = EFormat::NONE;
        uint32      Width           = 0;
        uint32      Height          = 0;
        uint32      BufferCount     = 0;
        uint32      SampleCount     = 0;
        bool        VerticalSync    = true;
    };

    class ISwapChain : public IDeviceChild
    {
    public:
        DECL_INTERFACE(ISwapChain);
        
        virtual bool ResizeBuffers(uint32 width, uint32 height) = 0;
        virtual void Present()                                  = 0;

        virtual SwapChainDesc   GetDesc()   const               = 0;
        virtual const Window*   GetWindow() const               = 0;
        virtual ITexture*       GetBuffer(uint32 bufferIndex)   = 0;
    };
}
