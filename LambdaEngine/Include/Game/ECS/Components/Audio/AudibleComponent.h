#pragma once

#include "ECS/Component.h"
#include <typeindex>

namespace LambdaEngine
{
	struct AudibleComponent
	{
		DECL_COMPONENT(AudibleComponent);
		GUID_Lambda SoundGUID;
	};
}