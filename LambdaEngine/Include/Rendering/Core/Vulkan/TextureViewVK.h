#pragma once
#include "Rendering/Core/API/ITextureView.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
    class TextureVK;
    class GraphicsDeviceVK;

    class TextureViewVK : public DeviceChildBase<GraphicsDeviceVK, ITextureView>
    {
        using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ITextureView>;
        
    public:
        TextureViewVK(const GraphicsDeviceVK* pDevice);
        ~TextureViewVK();
        
        bool Init(const TextureViewDesc& desc);
        
        FORCEINLINE VkImageView GetImageView() const
        {
            return m_ImageView;
        }
        
        //IDeviceChild interface
        virtual void SetName(const char* pName) override final;
        
        //ITextureView interface
        virtual ITexture* GetTexture() override final;
        
        FORCEINLINE virtual uint64 GetHandle() const override final
        {
            return (uint64)m_ImageView;
        }
        
        FORCEINLINE virtual TextureViewDesc GetDesc() const override final
        {
            return m_Desc;
        }
        
    private:
        TextureVK*  m_pTexture  = nullptr;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        
        TextureViewDesc m_Desc;
    };
}
