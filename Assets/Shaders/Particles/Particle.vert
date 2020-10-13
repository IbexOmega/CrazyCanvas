#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

const vec3 vertices[4] =
{
	vec3(-1.0, -1.0, 0.0),
	vec3(-1.0, 1.0, 0.0),
	vec3(1.0, -1.0, 0.0),
	vec3(1.0, 1.0, 0.0),
};

layout (binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{
	SPerFrameBuffer perFrameBuffer;
};

layout(binding = 0, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer Vertices
{ 
	SParticleVertex Val[]; 
} b_Vertices;

layout(binding = 1, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer ParticleInstances
{ 
	SParticle Val[]; 
} b_ParticleInstances;

layout (location = 0) out vec4 fragColor;

void main()
{
	SParticle particle = b_ParticleInstances.Val[gl_InstanceIndex];
	vec3 vertex = vertices[gl_VertexIndex];

	gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * particle.Transform * vec4(vertex, 1.0);
}