#pragma once
#include "Rendering/Core/API/ISampler.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class SamplerVK : public TDeviceChildBase<GraphicsDeviceVK, ISampler>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, ISampler>;

	public:
		SamplerVK(const GraphicsDeviceVK* pDevice);
		~SamplerVK();

		bool Init(const SamplerDesc* pDesc);

		FORCEINLINE VkSampler GetSampler() const
		{
			return m_Sampler;
		}

		// IDeviceChild Interface
		virtual void SetName(const char* pName) override final;

		// ISampler Interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_Sampler;
		}
        
        FORCEINLINE virtual SamplerDesc GetDesc() const override final
        {
            return m_Desc;
        }

	private:
		VkSampler   m_Sampler;
        SamplerDesc m_Desc;
	};
}
