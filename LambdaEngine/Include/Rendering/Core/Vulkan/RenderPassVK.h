#pragma once
#include "Rendering/Core/API/IRenderPass.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class RenderPassVK : public TDeviceChildBase<GraphicsDeviceVK, IRenderPass>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IRenderPass>;

		struct SubpassData
		{
			VkSubpassDescription	Subpass;
			VkAttachmentReference	InputAttachments[MAX_ATTACHMENTS];
			VkAttachmentReference	ColorAttachments[MAX_ATTACHMENTS];
			VkAttachmentReference	ResolveAttachments[MAX_ATTACHMENTS];
		};

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
		void CreateSubpassDescriptions(const RenderPassDesc& desc, SubpassData* pResultSubpasses);
		void CreateSubpassDependencies(const RenderPassDesc& desc, VkSubpassDependency* pResultSubpassDependencies);

	private:
		VkRenderPass	m_RenderPass;
		RenderPassDesc	m_Desc;
	};
}