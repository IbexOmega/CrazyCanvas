#include "Resources/GLSLShaderSource.h"
#include "Resources/GLSLang.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Shader.h"

#include "Rendering/RenderAPI.h"

#include <regex>

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

	Shader* GLSLShaderSource::Compile(const String& name, const String& defines)
	{
		EShLanguage shaderType = ConvertShaderStageToEShLanguage(m_Desc.ShaderStage);
		glslang::TShader shader(shaderType);

		const char* pSource = m_Desc.Source.c_str();

		shader.setPreamble(defines.c_str());
		shader.setStrings(&pSource, 1);

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
		
		DirStackFileIncluder includer;
		includer.pushExternalLocalDirectory(m_Desc.Directory);

		String preprocessedGLSL; 
		if (!shader.preprocess(pResources, defaultVersion, ENoProfile, false, false, messages, &preprocessedGLSL, includer))
		{
			LOG_ERROR("[GLSLShaderSource]: GLSL Preprocessing failed for: \"%s\"\nDefines:\n%s\n%s\n%s", m_Desc.Name.c_str(), defines.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		const char* pPreprocessedGLSL = preprocessedGLSL.c_str();
		shader.setStrings(&pPreprocessedGLSL, 1);

		if (!shader.parse(pResources, defaultVersion, false, messages))
		{
			LOG_ERROR("[GLSLShaderSource]: GLSL Parsing failed: \"%s\"\nDefines:\n%s\n%s\n%s", m_Desc.Name.c_str(), defines.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages))
		{
			LOG_ERROR("[GLSLShaderSource]: GLSL Linking failed: \"%s\"\nDefines:\n%s\n%s\n%s", m_Desc.Name.c_str(), defines.c_str(), shader.getInfoLog(), shader.getInfoDebugLog());
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