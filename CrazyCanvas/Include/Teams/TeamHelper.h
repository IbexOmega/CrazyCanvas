#pragma once

#include "Game/ECS/Components/Team/TeamComponent.h"

class TeamHelper
{
public:
	DECL_STATIC_CLASS(TeamHelper);

	static bool Init();

	static void SetTeamColor(uint8 teamIndex, const glm::vec3& color);
	static glm::vec3 GetTeamColor(uint8 teamIndex);
	
	//static GUID_Lambda GetTeamColorMaterialGUID(uint32 teamIndex);
	static glm::vec3 GetAvailableColor(uint32 colorIndex);
	static LambdaEngine::TArray<glm::vec3> GetAllAvailableColors();

	static glm::vec3 GetHSVColor(float angle);

	FORCEINLINE static GUID_Lambda GetTeamColorMaterialGUID(uint8 teamIndex)
	{
		uint8 index = teamIndex - 1;
		VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);
		return s_TeamColorMaterialGUIDs[index];
	}

	FORCEINLINE static GUID_Lambda GetTeamPlayerMaterialGUID(uint8 teamIndex)
	{
		uint8 index = teamIndex - 1;
		VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);
		return s_TeamPlayerMaterialGUIDs[index];
	}

	FORCEINLINE static GUID_Lambda GetMyTeamPlayerMaterialGUID() { return s_MyTeamPlayerMaterialGUID; }

private:
	inline static GUID_Lambda	s_PlayerTextureGUID = GUID_NONE;
	inline static GUID_Lambda	s_TeamColorMaterialGUIDs[LambdaEngine::MAX_NUM_TEAMS];

	inline static GUID_Lambda	s_TeamPlayerMaterialGUIDs[LambdaEngine::MAX_NUM_TEAMS];
	inline static GUID_Lambda	s_MyTeamPlayerMaterialGUID = GUID_NONE;
	inline static glm::vec3		s_TeamColors[LambdaEngine::MAX_NUM_TEAMS];
	
	inline static LambdaEngine::TArray<glm::vec3>	s_AvailableColors =
	{
		{1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, 0.0f},
		{1.0f, 0.0f, 1.0f},
		{1.0f, 0.5f, 0.75f},
		{0.05f, 0.05f, 0.05f},
	};
};