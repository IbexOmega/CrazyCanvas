#pragma once
#include "Rendering/Core/API/SBT.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class SBTVK : public TDeviceChildBase<GraphicsDeviceVK, SBT>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, SBT>;

	public:
		SBTVK(const GraphicsDeviceVK* pDevice);
		~SBTVK();

		bool Init(CommandList* pCommandList, const SBTDesc* pDesc);

		virtual bool Build(CommandList* pCommandList, TArray<DeviceChild*>& removedDeviceResources, const SBTDesc* pDesc) override;

		FORCEINLINE BufferVK* GetSBT()
		{
			return m_pSBTBuffer;
		}
		
		FORCEINLINE const VkStridedBufferRegionKHR* GetRaygenBufferRegion() const
		{
			return &m_RaygenBufferRegion;
		}

		FORCEINLINE const VkStridedBufferRegionKHR* GetHitBufferRegion() const
		{
			return &m_HitBufferRegion;
		}

		FORCEINLINE const VkStridedBufferRegionKHR* GetMissBufferRegion() const
		{
			return &m_MissBufferRegion;
		}

		FORCEINLINE const VkStridedBufferRegionKHR* GetCallableBufferRegion() const
		{
			return &m_CallableBufferRegion;
		}

		// DeviceChild interface
		virtual void SetName(const String& debugName) override final;

	private:
		BufferVK*				m_pShaderHandleStorageBuffer	= nullptr;
		BufferVK*				m_pSBTBuffer					= nullptr;
		BufferVK*				m_pShaderRecordsBuffer			= nullptr;
		uint32					m_NumShaderRecords				= 0;

		VkStridedBufferRegionKHR m_RaygenBufferRegion	= {};
		VkStridedBufferRegionKHR m_HitBufferRegion		= {};
		VkStridedBufferRegionKHR m_MissBufferRegion		= {};
		VkStridedBufferRegionKHR m_CallableBufferRegion	= {};
	};
}