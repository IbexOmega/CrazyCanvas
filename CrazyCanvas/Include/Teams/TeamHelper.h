#pragma once

#include "ECS/Components/Team/TeamComponent.h"

class TeamHelper
{
public:
	DECL_STATIC_CLASS(TeamHelper);

	static bool Init();
	
	static GUID_Lambda GetTeamColorMaterialGUID(uint32 teamIndex);
	static glm::vec3 GetTeamColor(uint32 teamIndex);

private:
	inline static GUID_Lambda s_TeamColorMaterialGUIDs[MAX_NUM_TEAMS];
};