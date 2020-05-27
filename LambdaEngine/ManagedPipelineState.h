#pragma once
#include "Rendering/Core/API/IPipelineState.h"

namespace LambdaEngine
{
	class ManagedGraphicsPipelineState
	{
	public:
		ManagedGraphicsPipelineState(const GraphicsPipelineStateDesc* pDesc);
		~ManagedGraphicsPipelineState();

		const GraphicsPipelineStateDesc* GetDesc();

	public:
		GraphicsPipelineStateDesc m_Desc;
	};
}