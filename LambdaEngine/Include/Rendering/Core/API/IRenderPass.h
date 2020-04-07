#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#define MAX_ATTACHMENTS 8
#define MAX_SUBPASSES 16
#define MAX_SUBPASS_DEPENDENCIES 16

namespace LambdaEngine 
{
	struct RenderPassAttachmentDesc
	{
		EFormat Format				= EFormat::NONE;
		uint32 SampleCount			= 0;
		ELoadOp LoadOp				= ELoadOp::NONE;
		EStoreOp StoreOp			= EStoreOp::NONE;
		ELoadOp StencilLoadOp		= ELoadOp::NONE;
		EStoreOp StencilStoreOp		= EStoreOp::NONE;
		ETextureState InitialState	= ETextureState::TEXTURE_STATE_UNKNOWN;
		ETextureState FinalState	= ETextureState::TEXTURE_STATE_UNKNOWN;
	};

	struct RenderPassSubpassDesc
	{
		ETextureState* pInputAttachmentStates		= nullptr;
		uint32 InputAttachmentCount					= 0;
		ETextureState* pColorAttachmentStates		= nullptr;
		ETextureState* pResolveAttachmentStates		= nullptr;
		uint32 ColorAttachmentCount					= 0;
		ETextureState DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_UNKNOWN;
	};

	struct RenderPassSubpassDependency
	{
		uint32 SrcSubpass					= 0xFFFFFFFF;
		uint32 DstSubpass					= 0xFFFFFFFF;
		FPipelineStageFlags SrcStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
		FPipelineStageFlags DstStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
		FAccessFlags SrcAccessMask			= FAccessFlags::ACCESS_FLAG_UNKNOWN;
		FAccessFlags DstAccessMask			= FAccessFlags::ACCESS_FLAG_UNKNOWN;
	};

	struct RenderPassDesc
	{
		const char* pName											= "RenderPass";
		const RenderPassAttachmentDesc* pAttachments				= nullptr;
		uint32 AttachmentCount										= 0;
		const RenderPassSubpassDesc* pSubpasses						= nullptr;
		uint32 SubpassCount											= 0;
		const RenderPassSubpassDependency* pSubpassDependencies		= nullptr;
		uint32 SubpassDependencyCount								= 0;
	};

	class IRenderPass : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IRenderPass);

		/*
		* Returns the API-specific handle to the underlaying resource
		*
		* return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64			GetHandle()	const = 0;
		virtual RenderPassDesc	GetDesc()	const = 0;
	};
}
