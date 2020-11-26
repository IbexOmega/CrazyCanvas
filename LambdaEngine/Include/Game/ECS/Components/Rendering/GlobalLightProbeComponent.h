#pragma once
#include "ECS/Component.h"

namespace LambdaEngine
{
	struct LightProbeData
	{
		uint32 SpecularResolution	= 128;
		uint32 DiffuseResolution	= 64;
	};

	struct GlobalLightProbeComponent
	{
		DECL_COMPONENT(GlobalLightProbeComponent);
		LightProbeData Data;
	};
}