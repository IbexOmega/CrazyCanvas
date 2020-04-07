#include "IDeviceChild.h"

namespace LambdaEngine
{
    class ITextureView;

    struct FrameBufferDesc
    {
        const char*             pName               = "";
        const ITextureView**    ppRenderTargets     = nullptr;
        uint32                  RenderTargetCount   = 0;
        const ITextureView*     pDepthStencil       = nullptr;
        uint32                  Width               = 0;
        uint32                  Height              = 0;
    };

    class IFrameBuffer : public IDeviceChild
    {
    public:
        DECL_DEVICE_INTERFACE(IFrameBuffer);

        /*
        * Retrives the depthstencil of this framebuffer. Calling this function increases the refcount.
        * This means that  the caller is responcible for calling release
        * 
        * return - A valid pointer to a ITextureView if successful otherwise zero
        */
        virtual ITextureView* GetDepthStencil() = 0;

        /*
        * Retrives a rendertarget from this framebuffer. The index should be the same as specified in
        * ppRenderTargets in when created. Calling this function increases the refcount.
        * This means that  the caller is responcible for calling release
        * 
        * index     - The index of the rendertarget 
        * return    - A valid pointer to a ITextureView if successful otherwise zero
        */
        virtual ITextureView*   GetRenderTarget(uint32 index)   = 0;
        virtual FrameBufferDesc GetDesc() const                 = 0;
    };
}
