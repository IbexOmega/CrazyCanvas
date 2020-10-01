#include "GUI/Core/GUIShaderManager.h"

#include "Resources/GLSLShaderSource.h"
#include "Resources/ResourceLoader.h"
#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	TArray<GUID_Lambda>	GUIShaderManager::s_GUIVertexShaders;
	TArray<GUID_Lambda>	GUIShaderManager::s_GUIPixelShaders;

	bool GUIShaderManager::Init()
	{
		if (!CreateVertexShaders())
		{
			LOG_ERROR("[GUIShaderManager]: Failed to create Vertex Shaders");
			return false;
		}

		if (!CreatePixelShaders())
		{
			LOG_ERROR("[GUIShaderManager]: Failed to create Pixel Shaders");
			return false;
		}

		return true;
	}

	GUID_Lambda GUIShaderManager::GetGUIVertexShaderGUID(uint32 index)
	{
		VALIDATE(index < s_GUIVertexShaders.GetSize());

		return s_GUIVertexShaders[index];
	}

	GUID_Lambda GUIShaderManager::GetGUIPixelShaderGUID(uint32 index)
	{
		VALIDATE(index < s_GUIPixelShaders.GetSize());

		return s_GUIPixelShaders[index];
	}

	bool GUIShaderManager::CreateVertexShaders()
	{
		#define VSHADER ""
		#define VSHADER_1(x0) "#define HAS_" #x0 "\n"
		#define VSHADER_2(x0, x1) "#define HAS_" #x0 "\n#define HAS_" #x1 "\n"
		#define VSHADER_3(x0, x1, x2) "#define HAS_" #x0 "\n#define HAS_" #x1 "\n#define HAS_" #x2 "\n"

		String pVertexShaderDefines[] =
		{
			VSHADER,
			VSHADER_1(COLOR),
			VSHADER_1(UV0),
			VSHADER_2(COLOR, COVERAGE),
			VSHADER_2(UV0, COVERAGE),
			VSHADER_2(COLOR, UV1),
			VSHADER_2(UV0, UV1),
			VSHADER_3(COLOR, UV1, UV2),
			VSHADER_3(UV0, UV1, UV2),
			VSHADER_3(COLOR, UV1, ST1),
			VSHADER_3(UV0, UV1, ST1)
		};

		GLSLShaderSource shaderSource = ResourceLoader::LoadShaderSourceFromFile("../Assets/Shaders/NoesisGUI/NoesisGUI.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER);

		for (uint32 vs = 0; vs < ARR_SIZE(pVertexShaderDefines); vs++)
		{
			String& defines = pVertexShaderDefines[vs];
			String name = "Noesis GUI Vertex Shader " + defines;
			GUID_Lambda shaderGUID = ResourceManager::RegisterShader(name, shaderSource.Compile(name, defines));

			s_GUIVertexShaders.PushBack(shaderGUID);
		}
		
		return true;
	}

	bool GUIShaderManager::CreatePixelShaders()
	{
		#define FSHADER(x) "#define EFFECT_" #x "\n"

		String pPixelShaderDefines[] =
		{
			FSHADER(RGBA),
			FSHADER(MASK),

			FSHADER(PATH_SOLID),
			FSHADER(PATH_LINEAR),
			FSHADER(PATH_RADIAL),
			FSHADER(PATH_PATTERN),

			FSHADER(PATH_AA_SOLID),
			FSHADER(PATH_AA_LINEAR),
			FSHADER(PATH_AA_RADIAL),
			FSHADER(PATH_AA_PATTERN),

			FSHADER(SDF_SOLID),
			FSHADER(SDF_LINEAR),
			FSHADER(SDF_RADIAL),
			FSHADER(SDF_PATTERN),

			FSHADER(IMAGE_OPACITY_SOLID),
			FSHADER(IMAGE_OPACITY_LINEAR),
			FSHADER(IMAGE_OPACITY_RADIAL),
			FSHADER(IMAGE_OPACITY_PATTERN),

			FSHADER(IMAGE_SHADOW_35V),
			FSHADER(IMAGE_SHADOW_63V),
			FSHADER(IMAGE_SHADOW_127V),

			FSHADER(IMAGE_SHADOW_35H_SOLID),
			FSHADER(IMAGE_SHADOW_35H_LINEAR),
			FSHADER(IMAGE_SHADOW_35H_RADIAL),
			FSHADER(IMAGE_SHADOW_35H_PATTERN),

			FSHADER(IMAGE_SHADOW_63H_SOLID),
			FSHADER(IMAGE_SHADOW_63H_LINEAR),
			FSHADER(IMAGE_SHADOW_63H_RADIAL),
			FSHADER(IMAGE_SHADOW_63H_PATTERN),

			FSHADER(IMAGE_SHADOW_127H_SOLID),
			FSHADER(IMAGE_SHADOW_127H_LINEAR),
			FSHADER(IMAGE_SHADOW_127H_RADIAL),
			FSHADER(IMAGE_SHADOW_127H_PATTERN),

			FSHADER(IMAGE_BLUR_35V),
			FSHADER(IMAGE_BLUR_63V),
			FSHADER(IMAGE_BLUR_127V),

			FSHADER(IMAGE_BLUR_35H_SOLID),
			FSHADER(IMAGE_BLUR_35H_LINEAR),
			FSHADER(IMAGE_BLUR_35H_RADIAL),
			FSHADER(IMAGE_BLUR_35H_PATTERN),

			FSHADER(IMAGE_BLUR_63H_SOLID),
			FSHADER(IMAGE_BLUR_63H_LINEAR),
			FSHADER(IMAGE_BLUR_63H_RADIAL),
			FSHADER(IMAGE_BLUR_63H_PATTERN),

			FSHADER(IMAGE_BLUR_127H_SOLID),
			FSHADER(IMAGE_BLUR_127H_LINEAR),
			FSHADER(IMAGE_BLUR_127H_RADIAL),
			FSHADER(IMAGE_BLUR_127H_PATTERN)
		};

		GLSLShaderSource shaderSource = ResourceLoader::LoadShaderSourceFromFile("../Assets/Shaders/NoesisGUI/NoesisGUI.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER);

		for (uint32 ps = 0; ps < ARR_SIZE(pPixelShaderDefines); ps++)
		{
			String& defines = pPixelShaderDefines[ps];
			String name = "Noesis GUI Pixel Shader " + defines;
			GUID_Lambda shaderGUID = ResourceManager::RegisterShader(name, shaderSource.Compile(name, defines));

			s_GUIPixelShaders.PushBack(shaderGUID);
		}

		return true;
	}
}