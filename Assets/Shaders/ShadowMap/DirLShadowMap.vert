#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable


layout(binding = 0, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = DRAW_SET_INDEX) restrict readonly buffer Vertices     { SVertex val[]; }          b_Vertices;
layout(binding = 1, set = DRAW_SET_INDEX) restrict readonly buffer Instances    { SInstance val[]; }        b_Instances;

layout(location = 0) out flat uint out_MaterialSlot;
layout(location = 1) out vec3 out_Normal;
layout(location = 2) out vec3 out_Tangent;
layout(location = 3) out vec3 out_Bitangent;
layout(location = 4) out vec2 out_TexCoord;
layout(location = 5) out vec4 out_ClipPosition;
layout(location = 6) out vec4 out_PrevClipPosition;

void main()
{
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SInstance instance                          = b_Instances.val[gl_InstanceIndex];
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    vec4 worldPosition      = instance.Transform * vec4(vertex.Position.xyz, 1.0f);
    vec4 prevWorldPosition  = instance.PrevTransform * vec4(vertex.Position.xyz, 1.0f);

    out_MaterialSlot            = instance.MaterialSlot;
	out_Normal 			        = normal;
	out_Tangent 		        = tangent;
	out_Bitangent 		        = bitangent;
	out_TexCoord 		        = vertex.TexCoord.xy;
    out_ClipPosition            = perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
    out_PrevClipPosition        = perFrameBuffer.PrevProjection * perFrameBuffer.PrevView * prevWorldPosition;

    gl_Position = out_ClipPosition;
}