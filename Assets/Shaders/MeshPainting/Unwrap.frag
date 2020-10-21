#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(location = 0) in vec3		in_WorldPosition;
layout(location = 1) in vec3		in_Normal;
layout(location = 2) in vec3		in_TargetPosition;
layout(location = 3) in vec3		in_TargetDirection;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_BrushMaskTexture;

layout(binding = 0, set = UNWRAP_DRAW_SET_INDEX) uniform UnwrapData				{ SUnwrapData val; }		u_UnwrapData;

layout(location = 0) out vec4   out_UnwrappedTexture;

float random (in vec3 x) {
	return fract(sin(dot(x, vec3(12.9898,78.233, 37.31633)))* 43758.5453123);
}

void reset()
{
	float alpha 	= out_UnwrappedTexture.a;
	uint data 		= floatBitsToUint(alpha);
	data = data >> 8;

	uint paintMode 	= data & 0x1;
	uint teamMode 	= (data >> 1) & 0x7F;

	data = data << 8;
	if (u_UnwrapData.val.PaintMode == 1)
	{
		if (u_UnwrapData.val.TeamMode == 0)
			out_UnwrappedTexture = vec4(1.f, 0.f, 0.f, uintBitsToFloat(data));
		else
			out_UnwrappedTexture = vec4(0.f, 0.f, 1.f, uintBitsToFloat(data));
	}
	else
	{
		out_UnwrappedTexture = vec4(0.f, 0.f, 0.f, uintBitsToFloat(data));
	}
}

void main()
{
	if (u_UnwrapData.val.Reset > 0)
		reset();

	const vec3 GLOBAL_UP	= vec3(0.f, 1.f, 0.f);
	const float BRUSH_SIZE	= 0.2f;
	const float PAINT_DEPTH = BRUSH_SIZE*2.0f;

	vec3 worldPosition		= in_WorldPosition;
	vec3 normal 			= normalize(in_Normal);
	vec3 targetPosition		= in_TargetPosition;
	vec3 direction			= normalize(in_TargetDirection);

	//targetPosition = targetPosition - direction;

	vec3 targetPosToWorldPos = worldPosition-targetPosition;

	float valid = step(0.f, dot(normal, -direction)); // Checks if looking from infront, else 0
	float len = abs(dot(targetPosToWorldPos, direction));
	valid *= 1.0f - step(PAINT_DEPTH, len);
	vec3 projectedPosition = targetPosition + len * direction;

	// Calculate uv-coordinates for a square encapsulating the sphere.
	vec3 up = GLOBAL_UP;
	if(abs(abs(dot(direction, up))-1.0f) < EPSILON)
		up = vec3(0.f, 0.f, 1.f);
	vec3 right	= normalize(cross(direction, up));
	up			= normalize(cross(right, direction));

	float u		= (dot(-targetPosToWorldPos, right)/BRUSH_SIZE*1.5f)*0.5f+0.5f;
	float v		= (dot(-targetPosToWorldPos, up)/BRUSH_SIZE*1.5f)*0.5f+0.5f;
	vec2 maskUV = vec2(u, v);

	// Apply brush mask
	vec4 brushMask = texture(u_BrushMaskTexture, maskUV).rgba;

	if(brushMask.a > EPSILON && maskUV.x > 0.f && maskUV.x < 1.f && maskUV.y > 0.f && maskUV.y < 1.f && valid > 0.5f)
	{
		// Paint mode 1 is normal paint. Paint mode 0 is remove paint (See enum in PaintMaskRenderer.h for enum)
		uint client;
		uint server;

		if (u_UnwrapData.val.RemoteMode == 0)
		{
			client = u_UnwrapData.val.TeamMode << 1;
			client |= u_UnwrapData.val.PaintMode;
		}
		else
		{
			server = u_UnwrapData.val.TeamMode << 1;
			server |= u_UnwrapData.val.PaintMode;
		}

		uint data = server << 8;
		data |= client;

		if (u_UnwrapData.val.PaintMode == 1)
		{
			if (u_UnwrapData.val.TeamMode == 0)
				out_UnwrappedTexture = vec4(1.f, 0.f, 0.f, uintBitsToFloat(data));
			else
				out_UnwrappedTexture = vec4(0.f, 0.f, 1.f, uintBitsToFloat(data));
		}
		else
		{
			out_UnwrappedTexture = vec4(0.f, 0.f, 0.f, uintBitsToFloat(data));
		}

	}
	else
		discard;
}
