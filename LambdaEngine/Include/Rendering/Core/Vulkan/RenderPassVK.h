#pragma once

#include "Rendering/Core/API/IRenderPass.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class RenderPassVK : public DeviceChildBase<GraphicsDeviceVK, IRenderPass>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IRenderPass>;

	public:
		RenderPassVK(const GraphicsDeviceVK* pDevice);
		~RenderPassVK();

		bool Init(const RenderPassDesc& desc);

		FORCEINLINE VkRenderPass GetRenderPass() const
		{
			return m_RenderPass;
		}

		// Inherited via IDeviceChild
		virtual void SetName(const char* pName) override final;

		// Inherited via IRenderPass
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_RenderPass;
		}

		FORCEINLINE virtual RenderPassDesc GetDesc() const override final
		{
			return m_Desc;
		}

	private:
		void CreateAttachmentDescriptions(const RenderPassDesc& desc, VkAttachmentDescription* pResultAttachments);
		void CreateSubpassDescriptions(const RenderPassDesc& desc, VkSubpassDescription* pResultSubpasses);
		void CreateSubpassDependencies(const RenderPassDesc& desc, VkSubpassDependency* pResultSubpassDependencies);

	private:
		VkRenderPass	m_RenderPass;
		RenderPassDesc	m_Desc;
	};
}