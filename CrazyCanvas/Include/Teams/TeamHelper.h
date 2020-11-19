#pragma once

#include "ECS/Components/Team/TeamComponent.h"
#include <array>

constexpr const uint32 MAX_NUM_COLORS = 6;

class TeamHelper
{
public:
	DECL_STATIC_CLASS(TeamHelper);

	static bool Init();

	static void SetTeamColor(uint32 teamIndex, const glm::vec3& color);
	static glm::vec3 GetTeamColor(uint32 teamIndex);
	
	static GUID_Lambda GetTeamColorMaterialGUID(uint32 teamIndex);
	static glm::vec3 GetAvailableColor(uint32 colorIndex);
	static LambdaEngine::TArray<glm::vec3> GetAllAvailableColors();

	static glm::vec3 GetHSVColor(float angle);

private:
	inline static glm::vec3		s_TeamColors[MAX_NUM_TEAMS];
	inline static GUID_Lambda	s_TeamColorMaterialGUIDs[MAX_NUM_TEAMS];
	
	inline static LambdaEngine::TArray<glm::vec3>	s_AvailableColors =
	{
		{1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, 0.0f},
		{1.0f, 0.0f, 1.0f},
		{1.0f, 0.5f, 0.75f},
		{0.0f, 0.0f, 0.0f},
	};
};