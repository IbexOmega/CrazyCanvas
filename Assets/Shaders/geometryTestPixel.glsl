#version 450
#extension GL_ARB_separate_shader_objects : enable

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
	vec4 Right;
	vec4 Up;
};

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_Tangent;
layout(location = 2) in vec3 in_Bitangent;
layout(location = 3) in vec2 in_TexCoord;
layout(location = 4) in vec4 in_Position;

layout(binding = 5, set = 0) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) out vec4 out_Color;

void main()
{
    out_Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}