#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../Noise.glsl"

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer					{ SPerFrameBuffer val; }	u_PerFrameBuffer;
layout(binding = 3, set = BUFFER_SET_INDEX) uniform HitPointsBuffer					{ SUnwrapData val[10]; }	u_HitPointsBuffer;

layout(binding = 0, set = DRAW_SET_INDEX) restrict buffer Vertices					{ SVertex val[]; }			b_Vertices;
layout(binding = 1, set = DRAW_SET_INDEX) restrict readonly buffer Instances		{ SInstance val[]; }		b_Instances;
layout(binding = 2, set = DRAW_SET_INDEX) restrict readonly buffer Meshlets			{ SMeshlet Val[]; } 		b_Meshlets;
layout(binding = 3, set = DRAW_SET_INDEX) restrict readonly buffer UniqueIndices	{ uint Val[]; } 			b_UniqueIndices;
layout(binding = 4, set = DRAW_SET_INDEX) restrict readonly buffer PrimitiveIndices	{ uint Val[]; } 			b_PrimitiveIndices;

layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_BrushMaskTexture;

layout(location = 0) out flat uint out_MaterialSlot;
layout(location = 1) out vec3 out_WorldPosition;
layout(location = 2) out vec3 out_Normal;
layout(location = 3) out vec3 out_Tangent;
layout(location = 4) out vec3 out_Bitangent;
layout(location = 5) out vec2 out_TexCoord;
layout(location = 6) out vec4 out_ClipPosition;
layout(location = 7) out vec4 out_PrevClipPosition;
layout(location = 8) out flat uint out_PaintInfo;
layout(location = 9) out float out_PaintDist;

vec2 rotate(in vec2 v, float a)
{
    float c = cos(a);
    float s = sin(a);
    mat2 r = mat2(vec2(c, s), vec2(-s, c));
    return r * v;
}

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

	uint instanceTeam = instance.TeamIndex;
	float paintDist = vertex.Normal.w; // Distance from target. 0 is at the target, 1 is at the edge.

	uint paintCount = uint(u_HitPointsBuffer.val[0].TargetPosition.w);
	for (uint hitPointIndex = 0; hitPointIndex < paintCount; hitPointIndex++)
	{
		SUnwrapData unwrapData = u_HitPointsBuffer.val[hitPointIndex];

		const vec3 GLOBAL_UP	= vec3(0.0f, 1.0f, 0.0f);
		const float BRUSH_SIZE	= 3.0f;
		const float PAINT_DEPTH = BRUSH_SIZE * 2.0f;

		vec3 normal 			= normalize(normal);
		vec3 targetPosition		= unwrapData.TargetPosition.xyz;
		vec3 direction			= normalize(unwrapData.TargetDirectionXYZAngleW.xyz);

		vec3 targetPosToWorldPos = worldPosition.xyz-targetPosition;

		uint teamMode = unwrapData.TeamMode;
		uint paintMode = unwrapData.PaintMode;
		uint remoteMode = unwrapData.RemoteMode;

		float valid = step(0.0f, dot(normal, -direction)); // Checks if looking from infront, else 0
		float len = abs(dot(targetPosToWorldPos, direction));
		valid *= 1.0f - step(PAINT_DEPTH, len);
		vec3 projectedPosition = targetPosition + len * direction;

		// Calculate uv-coordinates for a square encapsulating the sphere.
		vec3 up = GLOBAL_UP;
		if(abs(abs(dot(direction, up)) - 1.0f) < EPSILON)
			up = vec3(0.0f, 0.0f, 1.0f);
		vec3 right	= normalize(cross(direction, up));
		up			= normalize(cross(right, direction));

		float u		= (dot(-targetPosToWorldPos, right) / BRUSH_SIZE * 1.5f) * 0.5f + 0.5f;
		float v		= (dot(-targetPosToWorldPos, up) / BRUSH_SIZE * 1.5f) * 0.5f + 0.5f;
		vec2 maskUV = vec2(u, v);
		maskUV = rotate(maskUV-0.5f, unwrapData.TargetDirectionXYZAngleW.a)+0.5f;

		// Do not paint if they are in the same team. But they can remove paint.
		float isRemove = 1.f - step(0.5f, float(paintMode));
		float isSameTeam = 1.f - step(0.5f, abs(float(instanceTeam) - float(teamMode)));
		valid *= isRemove + (1.f - isRemove)*(1.f - isSameTeam);

		vec3 lineToPaint = targetPosToWorldPos;

		// Apply brush mask
		vec4 brushMask = texture(u_BrushMaskTexture, maskUV).rgba;

		vec2 st = maskUV-0.5f/* + vec2(-dot(targetPosition, right), -dot(targetPosition, up))*/;
		float a = atan(st.y, st.x);
		float r = length(st);
		float n = snoise(vec3(sin(a*1.548f)))*8.864f;
		float b = 0.108f;
		float dist = r-b;

		if(/*brushMask.a > EPSILON &&*/ maskUV.x > 0.0f && maskUV.x < 1.0f && maskUV.y > 0.0f && maskUV.y < 1.0f && valid > 0.5f)
		{
			// Paint mode 1 is normal paint. Paint mode 0 is remove paint (See enum in MeshPaintTypes.h for enum)
			uint teamSC = floatBitsToUint(vertex.Position.w);
			uint client = (teamSC >> 4) & 0x0F;
			uint server = teamSC & 0x0F;

			// Client
			if (remoteMode == 1)
			{
				client = (teamMode * paintMode) & 0x0F;
			}
			// Server
			else if (remoteMode == 2)
			{
				server = (teamMode * paintMode) & 0x0F;
			}

			teamSC = (client << 4) | server;
			vertex.Position.w = uintBitsToFloat(teamSC);

			lineToPaint = vec3(0.f);
		}

		//float t = clamp(length(lineToPaint) / BRUSH_SIZE, 0.f, 1.0f);
		vertex.Normal.w = min(vertex.Normal.w, dist/*smoothstep(0.f, 1.f, t)*/);
		paintDist = min(paintDist, vertex.Normal.w);
	}

	// Update vertex
	b_Vertices.val[gl_VertexIndex] = vertex;

	out_MaterialSlot		= instance.MaterialSlot;
	out_WorldPosition		= worldPosition.xyz;
	out_Normal				= normal;
	out_Tangent				= tangent;
	out_Bitangent			= bitangent;
	out_TexCoord			= vertex.TexCoord.xy;
	out_ClipPosition		= perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
	out_PrevClipPosition	= perFrameBuffer.PrevProjection * perFrameBuffer.PrevView * prevWorldPosition;
   	//out_ExtensionIndex		= instance.ExtensionGroupIndex * instance.TexturesPerExtensionGroup;
	out_PaintInfo 			= floatBitsToUint(vertex.Position.w);
	out_PaintDist 			= paintDist;

	gl_Position = out_ClipPosition;
}