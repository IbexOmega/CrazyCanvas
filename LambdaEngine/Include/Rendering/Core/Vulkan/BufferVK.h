#pragma once
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
    class GraphicsDeviceVK;

    class BufferVK : public DeviceChildBase<GraphicsDeviceVK, IBuffer>
    {
        using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IBuffer>;

    public:
        BufferVK(const GraphicsDeviceVK* pDevice);
        ~BufferVK();
        
        bool Init(const BufferDesc& desc);
        
        virtual void*   Map()   override final;
        virtual void    Unmap() override final;
        
        virtual void SetName(const char* pName) override final;
        
        FORCEINLINE virtual BufferDesc GetDesc() const override final
        {
            return m_Desc;
        }
        
    private:
        VkBuffer        m_Buffer    = VK_NULL_HANDLE;
        VkDeviceMemory  m_Memory    = VK_NULL_HANDLE;
        BufferDesc      m_Desc;
    };
}
