#pragma once
#include "Core/TSharedRef.h"

#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Rendering/Core/Vulkan/BufferVK.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class AccelerationStructureVK : public TDeviceChildBase<GraphicsDeviceVK, AccelerationStructure>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, AccelerationStructure>;

	public:
		AccelerationStructureVK(const GraphicsDeviceVK* pDevice);
		~AccelerationStructureVK();

		bool Init(const AccelerationStructureDesc* pDesc);

		FORCEINLINE VkAccelerationStructureKHR GetAccelerationStructure() const
		{
			return m_AccelerationStructure;
		}

		FORCEINLINE BufferVK* GetStorageBuffer()
		{
			return m_ScratchBuffer.Get();
		}

		FORCEINLINE BufferVK* GetScratchBuffer()
		{
			return m_ScratchBuffer.Get();
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// AccelerationStructure interface
		FORCEINLINE virtual uint64 GetDeviceAddress() const override final
		{
			return m_AccelerationStructureBuffer->GetDeviceAddress();
		}

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_AccelerationStructure);
		}

		FORCEINLINE virtual uint32 GetMaxInstanceCount() const override final
		{
			return m_MaxInstanceCount;
		}

	private:
		VkAccelerationStructureKHR	m_AccelerationStructure = VK_NULL_HANDLE;
		TSharedRef<BufferVK>		m_AccelerationStructureBuffer;
		TSharedRef<BufferVK>		m_ScratchBuffer;
		uint32						m_MaxInstanceCount;
	};
}
