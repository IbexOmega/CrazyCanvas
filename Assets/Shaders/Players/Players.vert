#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../MeshPaintFunc.glsl"

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer				{ SPerFrameBuffer val; }	u_PerFrameBuffer;

layout(binding = 0, set = DRAW_SET_INDEX) restrict readonly buffer Vertices		{ SVertex val[]; }			b_Vertices;
layout(binding = 1, set = DRAW_SET_INDEX) restrict readonly buffer Instances	{ SInstance val[]; }		b_Instances;

layout(location = 0) out flat uint out_MaterialSlot;
layout(location = 1) out vec3 out_WorldPosition;
layout(location = 2) out vec3 out_Normal;
layout(location = 3) out vec3 out_Tangent;
layout(location = 4) out vec3 out_Bitangent;
layout(location = 5) out vec2 out_TexCoord;
layout(location = 6) out vec4 out_ClipPosition;
layout(location = 7) out vec4 out_PrevClipPosition;
layout(location = 8) out flat uint out_InstanceIndex;
layout(location = 9) out vec3 out_ViewDirection;

// Mesh painting
layout(location = 10) out vec4 out_PaintInfo4;
layout(location = 11) out float out_PaintDist;
layout(location = 12) out vec3 out_LocalPosition;

void main()
{
	SVertex vertex					= b_Vertices.val[gl_VertexIndex];
	SInstance instance				= b_Instances.val[gl_InstanceIndex];
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;

	vec4 worldPosition		= instance.Transform * vec4(vertex.Position.xyz, 1.0f);
	vec4 prevWorldPosition	= instance.PrevTransform * vec4(vertex.Position.xyz, 1.0f);

	mat4 normalTransform = instance.Transform;

	vec3 normal				= normalize((normalTransform * vec4(vertex.Normal.xyz, 0.0f)).xyz);
	vec3 tangent			= normalize((normalTransform * vec4(vertex.Tangent.xyz, 0.0f)).xyz);
	vec3 bitangent			= normalize(cross(normal, tangent));

	out_MaterialSlot		= instance.MaterialSlot;
	out_WorldPosition		= worldPosition.xyz;
	out_Normal				= normal;
	out_Tangent				= tangent;
	out_Bitangent			= bitangent;
	out_TexCoord			= vertex.TexCoord.xy;
	out_PrevClipPosition	= perFrameBuffer.PrevProjection * perFrameBuffer.PrevView * prevWorldPosition;
    out_InstanceIndex		= gl_InstanceIndex;
	out_ViewDirection		= normalize(vec3(perFrameBuffer.View[0][2], perFrameBuffer.View[1][2], perFrameBuffer.View[2][2]));
	
	out_PaintInfo4 			= PackedPaintInfoToVec4(PackPaintInfo(floatBitsToUint(vertex.Position.w)));
	out_PaintDist 			= vertex.Normal.w;
	out_LocalPosition		= vec3(vertex.Tangent.w, vertex.TexCoord.z, vertex.TexCoord.w); // Original vertex position

	out_ClipPosition		= perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
	gl_Position = out_ClipPosition;
}