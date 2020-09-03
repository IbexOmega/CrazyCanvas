#pragma once
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class RenderPassVK : public TDeviceChildBase<GraphicsDeviceVK, RenderPass>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, RenderPass>;

		struct SubpassData
		{
			VkSubpassDescription	Subpass;
			VkAttachmentReference	InputAttachments[MAX_COLOR_ATTACHMENTS];
			VkAttachmentReference	ColorAttachments[MAX_COLOR_ATTACHMENTS];
			VkAttachmentReference	ResolveAttachments[MAX_COLOR_ATTACHMENTS];
			VkAttachmentReference	DepthStencil;
		};

	public:
		RenderPassVK(const GraphicsDeviceVK* pDevice);
		~RenderPassVK();

		bool Init(const RenderPassDesc* pDesc);

		FORCEINLINE VkRenderPass GetRenderPass() const
		{
			return m_RenderPass;
		}

	public:
		// DeviceChild Interface
		virtual void SetName(const String& name) override final;

		// RenderPass Interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_RenderPass);
		}

	private:
		void CreateAttachmentDescriptions(const RenderPassDesc* pDesc, VkAttachmentDescription* pResultAttachments);
		void CreateSubpassDescriptions(const RenderPassDesc* pDesc, SubpassData* pResultSubpasses);
		void CreateSubpassDependencies(const RenderPassDesc* pDesc, VkSubpassDependency* pResultSubpassDependencies);

	private:
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};
}
