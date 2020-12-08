#include "Teams/TeamHelper.h"

#include "Resources/ResourceManager.h"

bool TeamHelper::Init()
{
	using namespace LambdaEngine;

	// Load player textures
	s_PlayerTextureGUID = ResourceManager::LoadTextureFromFile(
		"Player/CharacterAlbedo.png",
		EFormat::FORMAT_R8G8B8A8_UNORM,
		true, true);

	LOG_ERROR("-----------PLAYER TEXTURE %u", s_PlayerTextureGUID);

	// Create Material for "My" Team
	{
		MaterialProperties myTeamMaterialProperties = {};
		myTeamMaterialProperties.Albedo = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
		myTeamMaterialProperties.Roughness = 0.7f;

		s_MyTeamPlayerMaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"My Team Material",
			s_PlayerTextureGUID,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			myTeamMaterialProperties);
	}

	// Create materials
	for (uint8 teamColorIndex = 0; teamColorIndex < NUM_TEAM_COLORS_AVAILABLE; teamColorIndex++)
	{
		glm::vec3 color = GetAvailableColor(teamColorIndex);

		MaterialProperties materialProperties = {};
		materialProperties.Albedo = glm::vec4(color, 1.0f);

		s_TeamColorMaterialGUIDs[teamColorIndex] = ResourceManager::LoadMaterialFromMemory(
			"Team Color" + std::to_string(teamColorIndex + 1) + " Color Material",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		s_TeamPlayerMaterialGUIDs[teamColorIndex] = ResourceManager::LoadMaterialFromMemory(
			"Team Color" + std::to_string(teamColorIndex + 1) + " Player Material",
			s_PlayerTextureGUID,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			s_PlayerTextureGUID,
			materialProperties);
	}

	//Just set default team colors to their own team index NUM_TEAM_COLORS_AVAILABLE should always be larger than MAX_NUM_TEAMS
	for (uint8 teamIndex = 0; teamIndex < MAX_NUM_TEAMS; teamIndex++)
	{
		s_TeamColorIndices[teamIndex] = teamIndex;
	}

	return true;
}

const glm::vec3& TeamHelper::GetTeamColor(uint8 teamIndex)
{
	uint8 index = teamIndex - 1;
	VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);
	return s_AvailableColors[s_TeamColorIndices[index]];
}

const ImageSources& TeamHelper::GetTeamImage(uint8 teamIndex)
{
	uint8 index = teamIndex - 1;
	VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);
	return s_AvailableImageSources[s_TeamColorIndices[index]];
}

uint8 TeamHelper::GetTeamColorIndex(uint8 teamIndex)
{
	uint8 index = teamIndex - 1;
	VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);
	return s_TeamColorIndices[index];
}

void TeamHelper::SetTeamColor(uint8 teamIndex, uint8 colorIndex)
{
	uint8 index = teamIndex - 1;
	VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);
	VALIDATE(colorIndex < NUM_TEAM_COLORS_AVAILABLE);

	// Store Team Color
	s_TeamColorIndices[index] = colorIndex;
}

glm::vec3 TeamHelper::GetAvailableColor(uint32 colorIndex)
{
	VALIDATE(colorIndex < NUM_TEAM_COLORS_AVAILABLE);
	return s_AvailableColors[colorIndex];
}

const glm::vec3* TeamHelper::GetAllAvailableColors()
{
	return s_AvailableColors;
}

glm::vec3 TeamHelper::GetHSVColor(float angle)
{
	float32 baseAngle = 240.0f;
	return glm::vec3(baseAngle + angle, 1.0f, 1.0f);
}
