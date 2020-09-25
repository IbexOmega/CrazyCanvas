<<<<<<< HEAD
#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "Defines.glsl"

struct Vertex
{
	vec4 Position;
	vec4 Color;
};

layout (binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{
	SPerFrameBuffer perFrameBuffer;
};

layout (binding = 0, set = 1) restrict readonly buffer vertexBuffer
{
	Vertex vertices[];
};

layout (location = 0) out vec4 fragColor;


void main()
{
    fragColor = vertices[gl_VertexIndex].Color;
    gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * vec4(vertices[gl_VertexIndex].Position.xyz, 1.0);
=======
#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "Defines.glsl"

struct Vertex
{
	vec4 Position;
	vec4 Color;
};

layout (binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{
	SPerFrameBuffer perFrameBuffer;
};

layout (binding = 0, set = 1) restrict readonly buffer vertexBuffer
{
	Vertex vertices[];
};

layout (location = 0) out vec4 fragColor;


void main()
{
    fragColor = vertices[gl_VertexIndex].Color;
    gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * vec4(vertices[gl_VertexIndex].Position.xyz, 1.0);
>>>>>>> master
}