#pragma once
#include "Rendering/Core/API/ISampler.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class SamplerVK : public DeviceChildBase<GraphicsDeviceVK, ISampler>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ISampler>;

	public:
		SamplerVK(const GraphicsDeviceVK* pDevice);
		~SamplerVK();

		bool Init(const SamplerDesc& desc);

		FORCEINLINE VkSampler GetSampler()
		{
			return m_Sampler;
		}

		// Inherited via DeviceChildBase
		virtual void SetName(const char* pName) override final;

		// Inherited via ISampler
		FORCEINLINE virtual uint64 GetHandle() const override
		{
			return (uint64)m_Sampler;
		}

	private:
		VkSampler m_Sampler;

	};
}
