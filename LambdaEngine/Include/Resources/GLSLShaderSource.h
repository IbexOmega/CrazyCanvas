#pragma once

#include "Containers/String.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	class Shader;

	struct GLSLShaderSourceDesc
	{
		String				Source		= "";
		String				EntryPoint	= "";
		FShaderStageFlag	ShaderStage = FShaderStageFlag::SHADER_STAGE_FLAG_NONE;
	};

	class GLSLShaderSource
	{
	public:
		GLSLShaderSource(const GLSLShaderSourceDesc* pDesc);
		~GLSLShaderSource();

		Shader* Compile(const String& name, const TArray<const char*>& defines);

	private:
		GLSLShaderSourceDesc m_Desc = {};
	};
}