#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine 
{
	struct RenderPassAttachmentDesc
	{
		EFormat			Format			= EFormat::FORMAT_NONE;
		uint32			SampleCount		= 0;
		ELoadOp			LoadOp			= ELoadOp::LOAD_OP_NONE;
		EStoreOp		StoreOp			= EStoreOp::STORE_OP_NONE;
		ELoadOp			StencilLoadOp	= ELoadOp::LOAD_OP_NONE;
		EStoreOp		StencilStoreOp	= EStoreOp::STORE_OP_NONE;
		ETextureState	InitialState	= ETextureState::TEXTURE_STATE_UNKNOWN;
		ETextureState	FinalState		= ETextureState::TEXTURE_STATE_UNKNOWN;
	};

	struct RenderPassSubpassDesc
	{
		ETextureState*	pInputAttachmentStates		= nullptr;
		uint32			InputAttachmentCount		= 0;
		ETextureState*	pRenderTargetStates			= nullptr;
		ETextureState*	pResolveAttachmentStates	= nullptr;
		uint32			RenderTargetCount			= 0;
		ETextureState	DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_UNKNOWN;
	};

	struct RenderPassSubpassDependencyDesc
	{
		uint32				SrcSubpass		= EXTERNAL_SUBPASS;
		uint32				DstSubpass		= EXTERNAL_SUBPASS;
		FPipelineStageFlags SrcStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
		FPipelineStageFlags DstStageMask	= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
		uint32				SrcAccessMask	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
		uint32				DstAccessMask	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
	};

	struct RenderPassDesc
	{
		const char*								pName					= "";
		const RenderPassAttachmentDesc*			pAttachments			= nullptr;
		uint32									AttachmentCount			= 0;
		const RenderPassSubpassDesc*			pSubpasses				= nullptr;
		uint32									SubpassCount			= 0;
		const RenderPassSubpassDependencyDesc*	pSubpassDependencies	= nullptr;
		uint32									SubpassDependencyCount	= 0;
	};

	class IRenderPass : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IRenderPass);

		/*
		* Returns the API-specific handle to the underlaying resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64			GetHandle()	const = 0;
		virtual RenderPassDesc	GetDesc()	const = 0;
	};
}
