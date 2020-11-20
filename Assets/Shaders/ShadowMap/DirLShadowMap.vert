#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#define NO_TEXTURES

#include "../Defines.glsl"

layout(binding = 0, set = DRAW_SET_INDEX) restrict readonly buffer Vertices         { SVertex val[]; }      b_Vertices;
layout(binding = 1, set = DRAW_SET_INDEX) restrict readonly buffer Instances        { SInstance val[]; }    b_Instances;

layout(binding = 0, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;


void main()
{
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SInstance instance                          = b_Instances.val[gl_InstanceIndex];
    SLightsBuffer lightsBuffer                  = b_LightsBuffer.val;
    
    vec4 worldPosition      = instance.Transform * vec4(vertex.Position.xyz, 1.0f);
    gl_Position             = lightsBuffer.DirL_ProjView * worldPosition;
}