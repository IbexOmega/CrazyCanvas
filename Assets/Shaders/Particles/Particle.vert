#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout (binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{
	SPerFrameBuffer perFrameBuffer;
} u_PerFrameBuffer;

layout(binding = 0, set = DRAW_SET_INDEX) restrict readonly buffer Vertices
{ 
	SParticleVertex Val[]; 
} b_Vertices;

layout(binding = 1, set = DRAW_SET_INDEX) restrict readonly buffer ParticleInstances
{ 
	SParticle Val[]; 
} b_ParticleInstances;

layout(binding = 0, set = 3) restrict readonly buffer Atlases
{ 
	SAtlasData Val[]; 
} b_Atlases;

layout(location = 0) out vec2 out_TexCoords;
layout(location = 1) out flat uint out_AtlasIndex;

void main()
{
	SParticle 		particle 	= b_ParticleInstances.Val[gl_InstanceIndex];
	SParticleVertex vertex 		= b_Vertices.Val[gl_VertexIndex];
	SPerFrameBuffer frameBuffer = u_PerFrameBuffer.perFrameBuffer;
	SAtlasData 		atlasData 	= b_Atlases.Val[particle.AtlasIndex];
	out_AtlasIndex = particle.AtlasIndex;

	// Hardcoded for now
	vec2 uv = (vertex.Position.xy + 1.f) * 0.5f;
	uint tx = particle.AtlasIndex % atlasData.ColCount;
	uint ty = particle.AtlasIndex / atlasData.RowCount;
	vec2 factor = vec2(atlasData.TileFactorX, atlasData.TileFactorY);
	out_TexCoords = uv * factor + factor*vec2(float(tx), float(ty));

	vec3 camRightWorldSpace = vec3(frameBuffer.View[0][0], frameBuffer.View[1][0], frameBuffer.View[2][0]);
	vec3 camUpWorldSpace 	= vec3(frameBuffer.View[0][1], frameBuffer.View[1][1], frameBuffer.View[2][1]);

	vec3 vPosition = camUpWorldSpace * vertex.Position.y + camRightWorldSpace * vertex.Position.x;
	vPosition *= particle.Radius;

	gl_Position = frameBuffer.Projection * frameBuffer.View * particle.Transform * vec4(vPosition, 1.0);
}