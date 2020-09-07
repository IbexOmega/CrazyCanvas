#pragma once
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class SamplerVK : public TDeviceChildBase<GraphicsDeviceVK, Sampler>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, Sampler>;

	public:
		SamplerVK(const GraphicsDeviceVK* pDevice);
		~SamplerVK();

		bool Init(const SamplerDesc* pDesc);

		FORCEINLINE VkSampler GetSampler() const
		{
			return m_Sampler;
		}

	public:
		// DeviceChild Interface
		virtual void SetName(const String& name) override final;

		// Sampler Interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Sampler);
		}

	private:
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};
}
