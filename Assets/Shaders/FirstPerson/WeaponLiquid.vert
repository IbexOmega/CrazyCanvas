#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../MeshPaintFunc.glsl"

struct SWeaponData
{
	mat4 Model;
	vec3 PlayerPos;
};

layout(push_constant) uniform PushConstants
{
	mat4 DefaultTransform;
	uint TeamIndex;
    float WaveX;
    float WaveZ;
} u_PC;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer				{ SPerFrameBuffer val; }	u_PerFrameBuffer;
layout(binding = 4, set = BUFFER_SET_INDEX) uniform WeaponData					{ SWeaponData val; }		u_WeaponData;

// Draw arg data
layout(binding = 0, set = 2) restrict buffer Vertices{ SVertex val[]; }					b_Vertices;
layout(binding = 1, set = 2) restrict readonly buffer Instances { SInstance val[]; }	b_Instances;
layout(binding = 2, set = 2) restrict readonly buffer Meshlets {SMeshlet val[]; }		b_Meshlets; // Not used
layout(binding = 3, set = 2) restrict readonly buffer UniqueIndices {uint val[]; }		b_UniqueIndices; // Not used
layout(binding = 4, set = 2) restrict readonly buffer PrimitiveIndices {uint val[]; }	b_PrimitiveIndices; // Not used

layout(location = 0) out vec3 out_WorldPosition;
layout(location = 1) out vec3 out_Normal;
layout(location = 2) out vec3 out_Tangent;
layout(location = 3) out vec3 out_Bitangent;
layout(location = 4) out vec2 out_TexCoord;
layout(location = 5) out vec4 out_ClipPosition;
layout(location = 6) out vec4 out_PrevClipPosition;
layout(location = 7) out flat uint out_InstanceIndex;
layout(location = 8) out vec3 out_ViewDirection;
layout(location = 9) out vec3 out_Position;

void main()
{
	SVertex vertex					= b_Vertices.val[gl_VertexIndex];
	SInstance instance				= b_Instances.val[gl_InstanceIndex];
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;
	SWeaponData weaponData			= u_WeaponData.val;

	mat4 normalTransform    = instance.Transform;

	vec3 position 			= vertex.Position.xyz;
	vec4 worldPosition		= weaponData.Model * instance.Transform * u_PC.DefaultTransform * vec4(position, 1.0f);
	vec4 prevWorldPosition	= instance.PrevTransform * vec4(vertex.Position.xyz, 1.0f);

	vec3 normal				= normalize((normalTransform * vec4(vertex.Normal.xyz, 0.0f)).xyz);
	vec3 tangent			= normalize((normalTransform * vec4(vertex.Tangent.xyz, 0.0f)).xyz);
	vec3 bitangent			= normalize(cross(normal, tangent));

	out_WorldPosition		= weaponData.PlayerPos;
	out_Normal				= normal;
	out_Tangent				= tangent;
	out_Bitangent			= bitangent;
	out_TexCoord			= vertex.TexCoord.xy;
	out_PrevClipPosition	= perFrameBuffer.Projection * perFrameBuffer.View * prevWorldPosition;
	out_ViewDirection		= normalize(vec3(perFrameBuffer.View[0][2], perFrameBuffer.View[1][2], perFrameBuffer.View[2][2]));

	out_ClipPosition		= perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;

    // vertex offset from origo X: 0.8258, Y: 0.25395, Z: -0.066376
    vec3 offset = vec3(0.8258f, 0.25395f, -0.066376f);
    vec3 size = vec3(0.14715f, 0.14715f, 0.317593f);
    out_Position = (weaponData.Model * instance.Transform * vec4(position - offset, 0.f)).xyz + vec3(0.f, size.y, 0.f);
    out_Position.y /= size.y*2.f;
    out_Position.y += position.x*u_PC.WaveX + position.z*u_PC.WaveZ;

	gl_Position = out_ClipPosition;
}