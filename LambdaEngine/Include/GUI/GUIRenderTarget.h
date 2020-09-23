#pragma once

#include "NsRender/RenderTarget.h"

namespace LambdaEngine
{
	class GUIRenderTarget : public Noesis::RenderTarget
	{
	public:
		GUIRenderTarget();
		~GUIRenderTarget();

		virtual Noesis::Texture* GetTexture() override final;
	};
}