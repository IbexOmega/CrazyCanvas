#pragma once
#include "ECS/Component.h"

enum class IndicatorTypeGUI
{
	FLAG_INDICATOR
	//DROP_OFF_INDICATOR //example
};


struct ProjectedGUIComponent
{
	DECL_COMPONENT(ProjectedGUIComponent);
	IndicatorTypeGUI GUIType;
};
