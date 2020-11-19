#pragma once

#include "ECS/Component.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/ISoundInstance2D.h"

namespace LambdaEngine
{
	struct AudibleComponent
	{
		DECL_COMPONENT(AudibleComponent);
		THashTable<String, ISoundInstance3D*> SoundInstances3D;
		THashTable<String, ISoundInstance2D*> SoundInstances2D;
	};

}
