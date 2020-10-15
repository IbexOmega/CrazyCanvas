#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout (binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{
	SPerFrameBuffer perFrameBuffer;
} u_PerFrameBuffer;

layout(binding = 0, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer Vertices
{ 
	SParticleVertex Val[]; 
} b_Vertices;

layout(binding = 1, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer ParticleInstances
{ 
	SParticle Val[]; 
} b_ParticleInstances;

void main()
{
	SParticle particle = b_ParticleInstances.Val[gl_InstanceIndex];
	SParticleVertex vertex = b_Vertices.Val[gl_VertexIndex];
	SPerFrameBuffer frameBuffer = u_PerFrameBuffer.perFrameBuffer;

	vec3 vCenterPos = particle.Transform[3].xyz;

	vec3 camRightWorldSpace = vec3(frameBuffer.View[0][0], frameBuffer.View[1][0], frameBuffer.View[2][0]);
	vec3 camUpWorldSpace 	= vec3(frameBuffer.View[0][1], frameBuffer.View[1][1], frameBuffer.View[2][1]);

	vec3 vPosition = camUpWorldSpace * vertex.Position.y + camRightWorldSpace * vertex.Position.x;
	vPosition *= particle.Radius;

	gl_Position = frameBuffer.Projection * frameBuffer.View * particle.Transform * vec4(vPosition, 1.0);
}