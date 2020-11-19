#pragma once

#include "ECS/Components/Team/TeamComponent.h"

class TeamHelper
{
public:
	DECL_STATIC_CLASS(TeamHelper);

	static bool Init();

	static glm::vec3 GetTeamColor(uint8 teamIndex);

	FORCEINLINE static GUID_Lambda GetTeamColorMaterialGUID(uint8 teamIndex)
	{
		VALIDATE(teamIndex < MAX_NUM_TEAMS);
		return s_TeamColorMaterialGUIDs[teamIndex];
	}

	FORCEINLINE static GUID_Lambda GetTeamPlayerMaterialGUID(uint8 teamIndex)
	{
		VALIDATE(teamIndex < MAX_NUM_TEAMS);
		return s_TeamPlayerMaterialGUIDs[teamIndex];
	}

	FORCEINLINE static GUID_Lambda GetMyTeamPlayerMaterialGUID() { return s_MyTeamPlayerMaterialGUID; }

private:
	inline static GUID_Lambda s_PlayerTextureGUID = GUID_NONE;
	inline static GUID_Lambda s_TeamColorMaterialGUIDs[MAX_NUM_TEAMS];

	inline static GUID_Lambda s_TeamPlayerMaterialGUIDs[MAX_NUM_TEAMS];
	inline static GUID_Lambda s_MyTeamPlayerMaterialGUID = GUID_NONE;
};