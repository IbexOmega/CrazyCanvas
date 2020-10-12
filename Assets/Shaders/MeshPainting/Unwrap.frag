#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(location = 0) in vec3	in_WorldPosition;
layout(location = 1) in vec3	in_Normal;
layout(location = 2) in vec3	in_TargetPosition;
layout(location = 3) in vec3	in_TargetDirection;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_BrushMaskTexture;

layout(location = 0) out vec4   out_UnwrappedTexture;

float random (in vec3 x) {
	return fract(sin(dot(x, vec3(12.9898,78.233, 37.31633)))* 43758.5453123);
}

void main()
{
	const vec3 GLOBAL_UP	= vec3(0.f, 1.f, 0.f);
	float BRUSH_SIZE		= 0.2f;

	vec3 worldPosition		= in_WorldPosition;
	vec3 normal 			= normalize(in_Normal);
	vec3 targetPosition		= in_TargetPosition;
	vec3 direction			= normalize(in_TargetDirection);
	
	vec3 targetPosToWorldPos = worldPosition-targetPosition;

	float valid = step(0.f, dot(normal, direction)); // Checks if looking from infront, else 0
	float len = abs(dot(targetPosToWorldPos, direction));
	valid *= 1.0f - step(BRUSH_SIZE, len);
	vec3 projectedPosition = targetPosition + len * direction;

	// Calculate uv-coordinates for a square encapsulating the sphere.
	vec3 right	= normalize(cross(direction, GLOBAL_UP));
	vec3 up		= normalize(cross(right, direction));
	float u		= (dot(-targetPosToWorldPos, right)/BRUSH_SIZE)*0.5f+0.5f;
	float v		= (dot(-targetPosToWorldPos, up)/BRUSH_SIZE)*0.5f+0.5f;
	vec2 maskUV = vec2(u, v);

	// Apply brush mask
	vec4 brushMask = texture(u_BrushMaskTexture, maskUV).rgba;

	if(brushMask.a > 0.01f && length(worldPosition-projectedPosition) <= BRUSH_SIZE && valid > 0.5f)
		out_UnwrappedTexture = vec4(1.f, 1.f, 1.f, 1.f);
	else
		discard;
		// out_UnwrappedTexture = vec4(0.f, 0.f, 0.f, 1.f);
}
