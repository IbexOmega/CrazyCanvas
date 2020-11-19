#include "Teams/TeamHelper.h"

#include "Resources/ResourceManager.h"

bool TeamHelper::Init()
{
	using namespace LambdaEngine;

	float32 baseAngle = 240.0f;
	float32 deltaAngle = 360.0f / MAX_NUM_TEAMS;

	// Load player textures
	s_PlayerTextureGUID = ResourceManager::LoadTextureFromFile(
		"Player/CharacterAlbedo.png",
		EFormat::FORMAT_R8G8B8A8_UNORM,
		true, true);

	// Create Material for "My" Team
	{
		MaterialProperties myTeamMaterialProperties = {};
		myTeamMaterialProperties.Albedo = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);

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
	for (uint8 teamIndex = 0; teamIndex < MAX_NUM_TEAMS; teamIndex++)
	{
		glm::vec3 color = glm::rgbColor(glm::vec3(baseAngle + deltaAngle * float32(teamIndex), 1.0f, 1.0f));

		MaterialProperties materialProperties = {};
		materialProperties.Albedo = glm::vec4(color, 1.0f);

		s_TeamColorMaterialGUIDs[teamIndex] = ResourceManager::LoadMaterialFromMemory(
			"Team " + std::to_string(teamIndex) + " Color Material",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		s_TeamPlayerMaterialGUIDs[teamIndex] = ResourceManager::LoadMaterialFromMemory(
			"Team " + std::to_string(teamIndex) + " Player Material",
			s_PlayerTextureGUID,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			s_PlayerTextureGUID,
			materialProperties);
	}

	return true;
}

glm::vec3 TeamHelper::GetTeamColor(uint8 teamIndex)
{
	float32 baseAngle = 240.0f;
	float32 deltaAngle = 360.0f / MAX_NUM_TEAMS;

	return glm::rgbColor(glm::vec3(baseAngle + deltaAngle * float32(teamIndex), 1.0f, 1.0f));
}