#pragma once

#include "Rendering/Core/API/PipelineState.h"

#include "GUI/GUIHelpers.h"

#include "NsRender/RenderDevice.h"

namespace LambdaEngine
{
	class PipelineState;
	class RenderPassAttachmentDesc;
	class GUIRenderTarget;

	constexpr const uint32 NUM_NON_DYNAMIC_VARIATIONS = 4;
	constexpr const uint32 NUM_PIPELINE_STATE_VARIATIONS = NUM_NON_DYNAMIC_VARIATIONS;

	class GUIPipelineStateCache
	{
	public:
		DECL_STATIC_CLASS(GUIPipelineStateCache);

		static bool Init(RenderPassAttachmentDesc* pBackBufferAttachmentDesc);
		static bool Release();

		static PipelineState* GetPipelineState(uint32 index, bool colorEnable, bool blendEnable, const NoesisShaderData& shaderData);

		FORCEINLINE static const PipelineLayout* GetPipelineLayout() { return s_pPipelineLayout; }

	private:
		static bool InitPipelineLayout();
		static bool InitPipelineState(uint32 index, bool colorEnable, bool blendEnable);
		static bool InitPipelineState(uint32 index, bool colorEnable, bool blendEnable, PipelineState** ppPipelineState, const NoesisShaderData& shaderData);

		static uint32 CalculateSubIndex(bool colorEnable, bool blendEnable);

	private:
		static TArray<PipelineState*[NUM_PIPELINE_STATE_VARIATIONS]> s_PipelineStates;
		static RenderPass*		s_pDummyRenderPass;
		static PipelineLayout*	s_pPipelineLayout;
	};
}