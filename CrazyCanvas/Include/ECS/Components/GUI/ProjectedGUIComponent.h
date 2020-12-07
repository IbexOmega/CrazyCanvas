#pragma once
#include "ECS/Component.h"

enum class IndicatorTypeGUI
{
	FLAG_INDICATOR,
	PING_INDICATOR
};

struct ProjectedGUIComponent
{
	DECL_COMPONENT(ProjectedGUIComponent);
	IndicatorTypeGUI GUIType;
};
