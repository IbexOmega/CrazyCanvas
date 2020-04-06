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

		virtual void SetName(const char* pName) override final;

	private:
		void CreateAttachmentDescriptions(const RenderPassDesc& desc, VkAttachmentDescription* pResultAttachments);
		void CreateSubpassDescriptions(const RenderPassDesc& desc, VkSubpassDescription* pResultSubpasses);
		void CreateSubpassDependencies(const RenderPassDesc& desc, VkSubpassDependency* pResultSubpassDependencies);

	private:
		VkRenderPass m_RenderPass;
	};
}