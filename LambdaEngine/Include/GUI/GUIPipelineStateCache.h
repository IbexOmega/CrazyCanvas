#pragma once

#include "Rendering/Core/API/PipelineState.h"

#include "NsRender/RenderDevice.h"

namespace LambdaEngine
{
	class PipelineState;
	class GUIRenderTarget;

	class GUIPipelineStateCache
	{
		static constexpr const uint32 NUM_PIPELINE_STATE_VARIATIONS = 4;

		union GUIRenderState
		{
			struct
			{
				uint32 Invalidated		: 1;
				uint32 ScissorEnable	: 1;
				uint32 ColorEnable		: 1;
				uint32 BlendEnable		: 1;
				uint32 StencilEnable	: 1;
				uint32 StencilMode		: 2;
				uint32 StencilRef		: 8;
			} Comp;

			uint32 Value;

			GUIRenderState() : Value(0) {}

			GUIRenderState(Noesis::RenderState renderState, uint8 stencilRef)
			{
				Comp.Invalidated	= 0;
				Comp.ScissorEnable	= renderState.f.scissorEnable;
				Comp.ColorEnable	= renderState.f.colorEnable;
				Comp.BlendEnable	= renderState.f.blendMode == Noesis::BlendMode::SrcOver;
				Comp.StencilEnable	= renderState.f.stencilMode != Noesis::StencilMode::Disabled;
				Comp.StencilMode	= renderState.f.stencilMode;
				Comp.StencilRef		= stencilRef;
			}
		};

	public:
		DECL_STATIC_CLASS(GUIPipelineStateCache);

		static bool Init();
		static bool Release();

		static PipelineState* GetPipelineState(uint32 index, bool colorEnable, bool blendEnable);

	private:
		static bool InitDummyRenderTarget();
		static bool InitPipelineLayout();
		static bool InitPipelineState(uint32 index, bool colorEnable, bool blendEnable);
		static bool InitPipelineState(uint32 index, bool colorEnable, bool blendEnable, PipelineState** ppPipelineState);

	private:
		static TArray<PipelineState*[NUM_PIPELINE_STATE_VARIATIONS]> s_PipelineStates;
		static GUIRenderTarget* s_pDummyRenderTarget;
		static PipelineLayout* s_pPipelineLayout;
	};
}