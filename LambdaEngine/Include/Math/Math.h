#pragma once

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4201) //Disable nameless struct/union warning
	#pragma warning(disable : 4251) //Disable the DLL- linkage warning for now
#endif

#define GLM_FORCE_INLINE
#define GLM_FORCE_SSE2
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>
