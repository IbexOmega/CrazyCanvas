#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout (binding = 0, set = 0) uniform PerFrameBuffer
{
	SPerFrameBuffer perFrameBuffer;
} u_PerFrameBuffer;

layout(binding = 0, set = 2) restrict readonly buffer Vertices
{ 
	SParticleVertex Val[]; 
} b_Vertices;

layout(binding = 1, set = 2) restrict readonly buffer ParticleInstances
{ 
	SParticle Val[]; 
} b_ParticleInstances;

layout(binding = 2, set = 2) restrict readonly buffer EmitterInstances
{ 
	SEmitter Val[]; 
} b_EmitterInstances;

layout (binding = 3, set = 2) readonly restrict buffer EmitterIndices
{
	uint Val[];
} b_EmitterIndices;

layout(binding = 0, set = 3) restrict readonly buffer Atlases
{ 
	SAtlasData Val[]; 
} b_Atlases;

layout(location = 0) out vec2 out_TexCoords;
layout(location = 1) out flat uint out_AtlasIndex;
layout(location = 2) out vec4 in_EmitterColor;

void main()
{
	SParticle 		particle 	= b_ParticleInstances.Val[gl_InstanceIndex];
	SEmitter 		emitter 	= b_EmitterInstances.Val[b_EmitterIndices.Val[gl_InstanceIndex]];
	SParticleVertex vertex 		= b_Vertices.Val[gl_VertexIndex];
	SPerFrameBuffer frameBuffer = u_PerFrameBuffer.perFrameBuffer;
	SAtlasData 		atlasData 	= b_Atlases.Val[emitter.AtlasIndex];
	out_AtlasIndex = emitter.AtlasIndex;
	in_EmitterColor = emitter.Color;

	// Hardcoded for now
	vec2 uv = (vertex.Position.xy + 1.f) * 0.5f;
	uint tx = particle.TileIndex % atlasData.ColCount;
	uint ty = particle.TileIndex / atlasData.RowCount;
	vec2 factor = vec2(atlasData.TileFactorX, atlasData.TileFactorY);
	out_TexCoords = uv * factor + factor*vec2(float(tx), float(ty));

	vec3 camRightWorldSpace = vec3(frameBuffer.View[0][0], frameBuffer.View[1][0], frameBuffer.View[2][0]);
	vec3 camUpWorldSpace 	= vec3(frameBuffer.View[0][1], frameBuffer.View[1][1], frameBuffer.View[2][1]);

	vec3 vPosition = camUpWorldSpace * vertex.Position.y + camRightWorldSpace * vertex.Position.x;
	vPosition *= mix(particle.EndRadius, particle.BeginRadius, particle.CurrentLife / emitter.LifeTime);

	gl_Position = frameBuffer.Projection * frameBuffer.View * particle.Transform * vec4(vPosition, 1.0);
}