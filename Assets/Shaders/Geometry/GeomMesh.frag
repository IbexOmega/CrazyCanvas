#version 450
#extension GL_ARB_separate_shader_objects 	: enable
#extension GL_GOOGLE_include_directive 		: enable

#include "../Helpers.glsl"

// Output
layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec3 out_AO_Rough_Metal;
layout(location = 2) out vec4 out_Compact_Normals;
layout(location = 3) out vec2 out_Velocity;

// Per VertexData
layout(location = 0) in flat uint   in_MaterialSlot;
layout(location = 1) in vec3        in_Normal;
layout(location = 2) in vec3        in_Tangent;
layout(location = 3) in vec3        in_Bitangent;
layout(location = 4) in vec2        in_TexCoord;
layout(location = 5) in vec4        in_ClipPosition;
layout(location = 6) in vec4        in_PrevClipPosition;

void main()
{
	vec2 storedGeometricNormal	= DirToOct(vec3(0.0f, 0.0f, 1.0f));
	out_AO_Rough_Metal = vec3(1.0f, 1.0f, 1.0f);
	out_Compact_Normals = vec4(storedGeometricNormal, storedGeometricNormal);
	out_Velocity = vec2(0.0f, 0.0f);
	out_Albedo = vec4(in_Normal, 1.0f);
}