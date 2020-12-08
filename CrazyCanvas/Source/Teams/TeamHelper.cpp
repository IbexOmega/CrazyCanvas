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
	for (uint8 teamIndex = 0; teamIndex < MAX_NUM_TEAMS; teamIndex++)
	{
		glm::vec3 color = GetAvailableColor(teamIndex);

		MaterialProperties materialProperties = {};
		materialProperties.Albedo = glm::vec4(color, 1.0f);

		s_TeamColorMaterialGUIDs[teamIndex] = ResourceManager::LoadMaterialFromMemory(
			"Team " + std::to_string(teamIndex + 1) + " Color Material",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		s_TeamPlayerMaterialGUIDs[teamIndex] = ResourceManager::LoadMaterialFromMemory(
			"Team " + std::to_string(teamIndex + 1) + " Player Material",
			s_PlayerTextureGUID,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			s_PlayerTextureGUID,
			materialProperties);

		s_TeamIndexes[teamIndex] = teamIndex;
	}

	return true;
}

const glm::vec3& TeamHelper::GetTeamColor(uint8 teamIndex)
{
	uint8 index = teamIndex - 1;
	VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);
	return s_AvailableColors[s_TeamIndexes[index]];
}

const ImageSources& TeamHelper::GetTeamImage(uint8 teamIndex)
{
	uint8 index = teamIndex - 1;
	VALIDATE(index < LambdaEngine::MAX_NUM_TEAMS);

	return s_AvailableImageSources[s_TeamIndexes[index]];
}

void TeamHelper::SetTeamColor(uint8 teamIndex, uint8 colorIndex)
{
	using namespace LambdaEngine;

	uint8 index = teamIndex - 1;
	VALIDATE(index < MAX_NUM_TEAMS);
	VALIDATE(colorIndex < s_AvailableColors.GetSize());

	// Update Team Material - Can't change loaded material therefore needs to be unloaded then reloaded with new albedo
	ResourceManager::UnloadMaterial(s_TeamColorMaterialGUIDs[index]);
	ResourceManager::UnloadMaterial(s_TeamPlayerMaterialGUIDs[index]);

	MaterialProperties materialProperties = {};
	materialProperties.Albedo = glm::vec4(s_AvailableColors[colorIndex], 1.0f);
	materialProperties.Roughness = 0.5f;


	s_TeamColorMaterialGUIDs[index] = ResourceManager::LoadMaterialFromMemory(
		"Team " + std::to_string(teamIndex) + " Color Material",
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_NORMAL_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		materialProperties);

	s_TeamPlayerMaterialGUIDs[index] = ResourceManager::LoadMaterialFromMemory(
		"Team " + std::to_string(teamIndex) + " Player Material",
		s_PlayerTextureGUID,
		GUID_TEXTURE_DEFAULT_NORMAL_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		s_PlayerTextureGUID,
		materialProperties);

	// Store Team Color
	s_TeamIndexes[index] = colorIndex;
}

glm::vec3 TeamHelper::GetAvailableColor(uint32 colorIndex)
{
	VALIDATE(colorIndex < s_AvailableColors.GetSize());
	return s_AvailableColors[colorIndex];
}

LambdaEngine::TArray<glm::vec3> TeamHelper::GetAllAvailableColors()
{
	return s_AvailableColors;
}

glm::vec3 TeamHelper::GetHSVColor(float angle)
{
	float32 baseAngle = 240.0f;
	return glm::vec3(baseAngle + angle, 1.0f, 1.0f);

}
