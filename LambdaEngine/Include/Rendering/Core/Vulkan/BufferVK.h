#pragma once
#include "Rendering/Core/API/IBuffer.h"

#include "DeviceChildVK.h"
#include "Vulkan.h"

namespace LambdaEngine
{
    class GraphicsDeviceVK;

    class BufferVK : public IBuffer, public DeviceChildVK
    {
    public:
        BufferVK(const GraphicsDeviceVK* pDevice);
        ~BufferVK();
        
        bool Create(const BufferDesc& desc);
        
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
