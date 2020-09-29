#pragma once

#include "ECS/Component.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Resources/ResourceManager.h"
#include "Containers/TSharedPtr.h"

#include <typeindex>

namespace LambdaEngine
{
	struct AudibleComponent
	{
		DECL_COMPONENT(AudibleComponent);
		ISoundEffect3D* pSoundEffect;
		ISoundInstance3D* pSoundInstance;
	};
}