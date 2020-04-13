#pragma once
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
    class GraphicsDeviceVK;

    class BufferVK : public TDeviceChildBase<GraphicsDeviceVK, IBuffer>
    {
        using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IBuffer>;

    public:
        BufferVK(const GraphicsDeviceVK* pDevice);
        ~BufferVK();
        
        bool Init(const BufferDesc& desc);
        
        FORCEINLINE VkBuffer GetBuffer() const
        {
            return m_Buffer;
        }

        //IDeviceChild interface
        virtual void SetName(const char* pName) override final;

        //IBuffer interface
        virtual void*   Map()   override final;
        virtual void    Unmap() override final;
        
        virtual uint64 GetDeviceAdress()            const override final;
        virtual uint64 GetAlignmentRequirement()    const override final;

        FORCEINLINE virtual BufferDesc GetDesc() const override final
        {
            return m_Desc;
        }

        FORCEINLINE virtual uint64 GetHandle() const override final
        {
            return (uint64)m_Buffer;
        }

    private:
        VkBuffer        m_Buffer                = VK_NULL_HANDLE;
        VkDeviceMemory  m_Memory                = VK_NULL_HANDLE;
        VkDeviceAddress m_DeviceAddress         = 0;
        VkDeviceAddress m_AlignementRequirement = 0;
        BufferDesc      m_Desc;
    };
}
