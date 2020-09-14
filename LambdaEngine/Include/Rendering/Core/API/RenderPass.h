#pragma once
#include "DeviceChild.h"
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
		TArray<ETextureState>	InputAttachmentStates;
		TArray<ETextureState>	RenderTargetStates;
		TArray<ETextureState>	ResolveAttachmentStates;
		ETextureState			DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_UNKNOWN;
	};

	struct RenderPassSubpassDependencyDesc
	{
		uint32				SrcSubpass		= EXTERNAL_SUBPASS;
		uint32				DstSubpass		= EXTERNAL_SUBPASS;
		FPipelineStageFlags SrcStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;
		FPipelineStageFlags DstStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;
		FMemoryAccessFlags	SrcAccessMask	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
		FMemoryAccessFlags	DstAccessMask	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
	};

	struct RenderPassDesc
	{
		String									DebugName = "";
		TArray<RenderPassAttachmentDesc>		Attachments;
		TArray<RenderPassSubpassDesc>			Subpasses;
		TArray<RenderPassSubpassDependencyDesc>	SubpassDependencies;
	};

	class RenderPass : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(RenderPass);

		/*
		* Returns the API-specific handle to the underlaying resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;

		FORCEINLINE const RenderPassDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		RenderPassDesc m_Desc;
	};
}
