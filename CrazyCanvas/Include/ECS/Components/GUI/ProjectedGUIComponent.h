#pragma once
#include "ECS/Component.h"

enum class IndicatorTypeGUI
{
	FLAG_INDICATOR
	//PLAYER_NAME_INDICATOR
};


struct ProjectedGUIComponent
{
	DECL_COMPONENT(ProjectedGUIComponent);
	IndicatorTypeGUI GUIType;
};
