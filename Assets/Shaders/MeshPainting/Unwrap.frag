#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(location = 0) in vec3 in_WorldPosition;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_TargetPosition;
layout(location = 3) in vec3 in_TargetDirection;
layout(location = 4) in flat int in_InstanceTeam;

layout(push_constant) uniform FrameSettingBuffer
{
	layout(offset = 8) uint ShouldReset;
	layout(offset = 12) uint ShouldPaint;
	layout(offset = 16) uint PaintCount;
} p_FrameSettings;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_BrushMaskTexture;

layout(binding = 0, set = UNWRAP_DRAW_SET_INDEX) uniform UnwrapData
{ 
	SUnwrapData val[10]; 
} u_UnwrapData;

layout(location = 0, component = 0) out uint out_BitsServer;
layout(location = 0, component = 1) out uint out_BitsClient;

vec2 rotate(in vec2 v, float a)
{
    float c = cos(a);
    float s = sin(a);
    mat2 r = mat2(vec2(c, s), vec2(-s, c));
    return r * v;
}

void main()
{
	bool shouldDiscard = true;
	if (p_FrameSettings.ShouldReset == 1)
	{
		out_BitsClient	= 0;
		shouldDiscard	= false;
	}

	for (uint hitPointIndex = 0; hitPointIndex < p_FrameSettings.PaintCount; hitPointIndex++)
	{
		const vec3 GLOBAL_UP	= vec3(0.0f, 1.0f, 0.0f);
		const float BRUSH_SIZE	= 0.5f;
		const float PAINT_DEPTH = BRUSH_SIZE * 2.0f;

		vec3 worldPosition		= in_WorldPosition;
		vec3 normal 			= normalize(in_Normal);
		vec3 targetPosition		= in_TargetPosition;
		vec3 direction			= normalize(in_TargetDirection);

		vec3 targetPosToWorldPos = worldPosition-targetPosition;

		uint teamMode = u_UnwrapData.val[hitPointIndex].TeamMode;
		uint paintMode = u_UnwrapData.val[hitPointIndex].PaintMode;
		uint remoteMode = u_UnwrapData.val[hitPointIndex].RemoteMode;

		float valid = step(0.0f, dot(normal, -direction)); // Checks if looking from infront, else 0
		float len = abs(dot(targetPosToWorldPos, direction));
		valid *= 1.0f - step(PAINT_DEPTH, len);
		vec3 projectedPosition = targetPosition + len * direction;

		// Calculate uv-coordinates for a square encapsulating the sphere.
		vec3 up = GLOBAL_UP;
		if(abs(abs(dot(direction, up)) - 1.0f) < EPSILON)
			up = vec3(0.0f, 0.0f, 1.0f);
		vec3 right	= normalize(cross(direction, up));
		up			= normalize(cross(right, direction));

		float u		= (dot(-targetPosToWorldPos, right) / BRUSH_SIZE * 1.5f) * 0.5f + 0.5f;
		float v		= (dot(-targetPosToWorldPos, up) / BRUSH_SIZE * 1.5f) * 0.5f + 0.5f;
		vec2 maskUV = vec2(u, v);

		maskUV = rotate(maskUV-0.5f, u_UnwrapData.val[hitPointIndex].TargetDirectionXYZAngleW.a)+0.5f;

		// Apply brush mask
		vec4 brushMask = texture(u_BrushMaskTexture, maskUV).rgba;

		// Do not paint if they are in the same team. But they can remove paint.
		float isRemove = 1.f - step(0.5f, float(paintMode));
		float isSameTeam = 1.f - step(0.5f, abs(float(in_InstanceTeam) - float(teamMode)));
		valid *= isRemove + (1.f - isRemove)*(1.f - isSameTeam);

		// Only paint if the position is within the texture's uv coordinates.
		if(brushMask.a > EPSILON && maskUV.x > 0.0f && maskUV.x < 1.0f && maskUV.y > 0.0f && maskUV.y < 1.0f && valid > 0.5f)
		{
			// Paint mode 1 is normal paint. Paint mode 0 is remove paint (See enum in PaintMaskRenderer.h for enum)
			if (remoteMode == 1)
			{
				uint client = teamMode << 1;
				client |= paintMode & 0x1;
				out_BitsClient = client & 0xFF;
			}
			else if (remoteMode == 2)
			{
				uint server = teamMode << 1;
				server |= paintMode & 0x1;
				out_BitsServer = server & 0xFF;
			}

			shouldDiscard = false;
		}
	}

	if (shouldDiscard)
		discard;
}
