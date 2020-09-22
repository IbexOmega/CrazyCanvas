#version 450 core
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec4 Color;
};

layout (binding = 0) uniform PerFrameBuffer
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
} g_PerFrame;

layout (binding = 1) buffer vertexBuffer
{
	Vertex vertices[];
};

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = vertices[gl_VertexIndex].Color;
    gl_Position = g_PerFrame.View * g_PerFrame.Projection * vec4(vertices[gl_VertexIndex].Position.xyz, 1.0);
}