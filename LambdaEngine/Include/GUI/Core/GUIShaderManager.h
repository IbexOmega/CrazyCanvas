#pragma once

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	class GUIShaderManager
	{
	public:
		static bool Init();

		static GUID_Lambda GetGUIVertexShaderGUID(uint32 index);
		static GUID_Lambda GetGUIPixelShaderGUID(uint32 index);

	private:
		static bool CreateVertexShaders();
		static bool CreatePixelShaders();

	private:
		static TArray<GUID_Lambda>	s_GUIVertexShaders;
		static TArray<GUID_Lambda>	s_GUIPixelShaders;
	};
}