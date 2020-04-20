#version 450
#extension GL_ARB_separate_shader_objects : enable

struct SMaterialParameters
{
    vec4    Albedo;
    float   Ambient;
    float   Metallic;
    float   Roughness;
    float   Unreserved;
};

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 Position;
	vec4 Right;
	vec4 Up;
};

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_Tangent;
layout(location = 2) in vec3 in_Bitangent;
layout(location = 3) in vec2 in_TexCoord;
layout(location = 4) in vec4 in_Position;
layout(location = 5) in flat uint in_MaterialIndex;

layout(binding = 5 , set = 0) buffer MaterialParameters  { SMaterialParameters val[]; }  b_MaterialParameters;
layout(binding = 6 , set = 0) uniform sampler2D u_SceneAlbedoMaps[32];
layout(binding = 7 , set = 0) uniform sampler2D u_SceneNormalMaps[32];
layout(binding = 8 , set = 0) uniform sampler2D u_SceneAOMaps[32];
layout(binding = 9 , set = 0) uniform sampler2D u_SceneMetallicMaps[32];
layout(binding = 10, set = 0) uniform sampler2D u_SceneRougnessMaps[32];

layout(location = 0) out vec4 out_Color;

void main()
{
    out_Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	out_Color = texture(u_SceneAlbedoMaps[in_MaterialIndex], in_TexCoord);
}