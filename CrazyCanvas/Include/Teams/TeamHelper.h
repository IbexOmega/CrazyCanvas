#pragma once

#include "Game/ECS/Components/Team/TeamComponent.h"

struct ImageSources
{
	LambdaEngine::String PaintAmmo;
	LambdaEngine::String PaintAmmoDrop;
};

class TeamHelper
{
public:
	DECL_STATIC_CLASS(TeamHelper);

	static bool Init();

	static void SetTeamColor(uint8 teamIndex, uint8 colorIndex);
	static const glm::vec3& GetTeamColor(uint8 teamIndex);
	static const ImageSources& GetTeamImage(uint8 teamIndex);

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
	inline static uint8			s_TeamIndexes[LambdaEngine::MAX_NUM_TEAMS];

	inline static GUID_Lambda	s_PlayerTextureGUID = GUID_NONE;
	inline static GUID_Lambda	s_TeamColorMaterialGUIDs[LambdaEngine::MAX_NUM_TEAMS];

	inline static GUID_Lambda	s_MyTeamPlayerMaterialGUID = GUID_NONE;
	inline static GUID_Lambda	s_TeamPlayerMaterialGUIDs[LambdaEngine::MAX_NUM_TEAMS];
	inline static glm::vec3		s_TeamColors[LambdaEngine::MAX_NUM_TEAMS];

	inline static LambdaEngine::TArray<glm::vec3>	s_AvailableColors =
	{
		{1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, 0.0f},
		{1.0f, 0.0f, 1.0f},
		{1.0f, 0.5f, 0.75f}
	};


	inline static LambdaEngine::TArray<ImageSources> s_AvailableImageSources =
	{
		{
			"PaintAmmo/RedPaint.png", "PaintAmmo/RedPaintDrop.png"
		},
		{
			"PaintAmmo/BluePaint.png", "PaintAmmo/BluePaintDrop.png"
		},
		{
			"PaintAmmo/GreenPaint.png", "PaintAmmo/GreenPaintDrop.png"
		},
		{
			"PaintAmmo/YellowPaint.png", "PaintAmmo/YellowPaintDrop.png"
		},
		{
			"PaintAmmo/PurplePaint.png", "PaintAmmo/PurplePaintDrop.png"
		},
		{
			"PaintAmmo/PinkPaint.png", "PaintAmmo/PinkPaintDrop.png"
		}
	};

};