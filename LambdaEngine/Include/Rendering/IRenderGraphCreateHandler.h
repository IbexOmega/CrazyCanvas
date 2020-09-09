#pragma once

namespace LambdaEngine
{
	class RenderGraph;

	class IRenderGraphCreateHandler
	{
	public:
		DECL_INTERFACE(IRenderGraphCreateHandler);

		virtual void OnRenderGraphRecreate(RenderGraph* pRenderGraph) = 0;
	};
}