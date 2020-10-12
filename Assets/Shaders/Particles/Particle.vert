#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

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
	SParticle particle = b_ParticleInstances[gl_InstanceID]
	PVertex vertex = b_Vertices.Val[gl_VertexIndex];

	gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * particle.Transform * vec4(vertex.Position.xyz, 1.0);
}