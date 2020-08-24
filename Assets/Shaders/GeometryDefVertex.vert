#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "Defines.glsl"

layout (constant_id = 0) const uint OTHER_TEXTURES_IN_PASS                  = 0;
layout (constant_id = 1) const uint ALLOWED_TEXTURES_PER_DESCRIPTOR_SET     = 256;

layout(binding = 0, set = BUFFER_SET_INDEX) restrict readonly buffer Vertices               { SVertex val[]; }              b_Vertices;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer Indices                { uint val[]; }                 b_Indices;
layout(binding = 2, set = BUFFER_SET_INDEX) restrict readonly buffer PrimaryInstances       { SPrimaryInstance val[]; }     b_PrimaryInstances;
layout(binding = 3, set = BUFFER_SET_INDEX) restrict readonly buffer SecondaryInstances     { SSecondaryInstance val[]; }   b_SecondaryInstances;
layout(binding = 4, set = BUFFER_SET_INDEX) restrict readonly buffer MeshIndices            { SMeshIndexDesc val[]; }       b_MeshIndices;
layout(binding = 5, set = BUFFER_SET_INDEX) uniform PerFrameBuffer                          { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) out flat uint out_MaterialIndex;
layout(location = 1) out vec3 out_Normal;
layout(location = 2) out vec3 out_Tangent;
layout(location = 3) out vec3 out_Bitangent;
layout(location = 4) out vec3 out_LocalNormal;
layout(location = 5) out vec2 out_TexCoord;
layout(location = 6) out vec4 out_WorldPosition;
layout(location = 7) out vec4 out_ClipPosition;
layout(location = 8) out vec4 out_PrevClipPosition;

void main()
{
    SPrimaryInstance primaryInstance            = b_PrimaryInstances.val[gl_InstanceIndex];
    SSecondaryInstance secondaryInstance        = b_SecondaryInstances.val[gl_InstanceIndex];
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    uint meshIndexID                            = (primaryInstance.Mask_MeshMaterialIndex) & 0x00FFFFFF;
    SMeshIndexDesc meshIndexDesc                = b_MeshIndices.val[meshIndexID];

    //Calculate a 4x4 matrix so that we calculate the correct w for worldPosition
    mat4 transform;
	transform[0] = vec4(primaryInstance.Transform[0]);
	transform[1] = vec4(primaryInstance.Transform[1]);
	transform[2] = vec4(primaryInstance.Transform[2]);
	transform[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    transform = transpose(transform);

    mat4 prevTransform   = transpose(secondaryInstance.PrevTransform);

    vec4 worldPosition      = transform * vec4(vertex.Position.xyz, 1.0f);
    vec4 prevWorldPosition  = prevTransform * vec4(vertex.Position.xyz, 1.0f);

    vec3 normal 	= normalize((transform * vec4(vertex.Normal.xyz, 0.0f)).xyz);
	vec3 tangent    = normalize((transform * vec4(vertex.Tangent.xyz, 0.0f)).xyz);
	vec3 bitangent 	= normalize(cross(normal, tangent));
    vec2 texCoord   = vertex.TexCoord.xy;

    uint materialsRenderedPerPass = (ALLOWED_TEXTURES_PER_DESCRIPTOR_SET - OTHER_TEXTURES_IN_PASS) / 5;

    out_MaterialIndex           = meshIndexDesc.MaterialIndex % materialsRenderedPerPass;
	out_Normal 			        = normal;
	out_Tangent 		        = tangent;
	out_Bitangent 		        = bitangent;
    out_LocalNormal             = vertex.Normal.xyz;
	out_TexCoord 		        = texCoord;
	out_WorldPosition		    = worldPosition;
    out_ClipPosition            = perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
    out_PrevClipPosition        = perFrameBuffer.PrevProjection * perFrameBuffer.PrevView * prevWorldPosition;

    gl_Position = out_ClipPosition;
}
