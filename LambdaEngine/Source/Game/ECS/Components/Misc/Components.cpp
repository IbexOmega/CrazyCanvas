#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Misc/MaskComponent.h"
#include "Game/ECS/Components/Misc/MeshPaintComponent.h"

namespace LambdaEngine
{
	INIT_COMPONENT(TrackComponent);
	INIT_DRAW_ARG_COMPONENT(MaskComponent);
	INIT_DRAW_ARG_COMPONENT(MeshPaintComponent);
}