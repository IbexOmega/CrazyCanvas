#include "Resources/GLSLShaderSource.h"
#include "Resources/GLSLang.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Shader.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{
	GLSLShaderSource::GLSLShaderSource(const GLSLShaderSourceDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		m_Desc = *pDesc;
	}

	GLSLShaderSource::~GLSLShaderSource()
	{
	}

	Shader* GLSLShaderSource::Compile(const String& name, const TArray<const char*>& defines)
	{
		TArray<const char*> strings = defines;

		EShLanguage shaderType = ConvertShaderStageToEShLanguage(m_Desc.ShaderStage);
		glslang::TShader shader(shaderType);

		strings.PushBack(m_Desc.Source.c_str());
		shader.setStrings(strings.GetData(), strings.GetSize());

		//Todo: Fetch this
		int32 clientInputSemanticsVersion					= GetDefaultClientInputSemanticsVersion();
		glslang::EShTargetClientVersion vulkanClientVersion	= GetDefaultVulkanClientVersion();
		glslang::EShTargetLanguageVersion targetVersion		= GetDefaultSPIRVTargetVersion();
		const TBuiltInResource* pResources					= GetDefaultBuiltInResources();
		EShMessages messages								= GetDefaultMessages();
		int32 defaultVersion								= GetDefaultVersion();

		shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, clientInputSemanticsVersion);
		shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
		shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);
		
		if (!shader.parse(pResources, defaultVersion, false, messages))
		{
			const char* pShaderInfoLog		= shader.getInfoLog();
			const char* pShaderDebugInfo	= shader.getInfoDebugLog();
			LOG_ERROR("[ResourceLoader]: GLSL Parsing failed for: \"%s\"\n%s\n%s", pShaderInfoLog, pShaderDebugInfo);
			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages))
		{
			LOG_ERROR("[ResourceLoader]: GLSL Linking failed for: \"%s\"\n%s\n%s", shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		glslang::TIntermediate* pIntermediate = program.getIntermediate(shaderType);

		TArray<uint32> sourceSPIRV;

		spv::SpvBuildLogger logger;
		glslang::SpvOptions spvOptions;
		std::vector<uint32> std_sourceSPIRV;
		glslang::GlslangToSpv(*pIntermediate, std_sourceSPIRV, &logger, &spvOptions);
		sourceSPIRV.Assign(std_sourceSPIRV.data(), std_sourceSPIRV.data() + std_sourceSPIRV.size());

		const uint32 sourceSize = static_cast<uint32>(sourceSPIRV.GetSize()) * sizeof(uint32);

		ShaderDesc shaderDesc = { };
		shaderDesc.DebugName	= name;
		shaderDesc.Source		= TArray<byte>(reinterpret_cast<byte*>(sourceSPIRV.GetData()), reinterpret_cast<byte*>(sourceSPIRV.GetData()) + sourceSize);
		shaderDesc.EntryPoint	= m_Desc.EntryPoint;
		shaderDesc.Stage		= m_Desc.ShaderStage;
		shaderDesc.Lang			= EShaderLang::SHADER_LANG_SPIRV;

		return RenderAPI::GetDevice()->CreateShader(&shaderDesc);
	}
}