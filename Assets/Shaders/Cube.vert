#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_debug_printf : enable

#include "Defines.glsl"

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer                          { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) out vec3 out_WorldPosition;

void main() 
{
    const vec3 vertices[36] =
    {
        vec3(-1.0f, -1.0f, -1.0f),
        vec3(-1.0f, -1.0f,  1.0f),
        vec3(-1.0f,  1.0f,  1.0f),
        vec3( 1.0f,  1.0f, -1.0f), 
        vec3(-1.0f, -1.0f, -1.0f),
        vec3(-1.0f,  1.0f, -1.0f),
        vec3( 1.0f, -1.0f,  1.0f),
        vec3(-1.0f, -1.0f, -1.0f),
        vec3( 1.0f, -1.0f, -1.0f),
        vec3( 1.0f,  1.0f, -1.0f),
        vec3( 1.0f, -1.0f, -1.0f),
        vec3(-1.0f, -1.0f, -1.0f),
        vec3(-1.0f, -1.0f, -1.0f),
        vec3(-1.0f,  1.0f,  1.0f),
        vec3(-1.0f,  1.0f, -1.0f),
        vec3( 1.0f, -1.0f,  1.0f),
        vec3(-1.0f, -1.0f,  1.0f),
        vec3(-1.0f, -1.0f, -1.0f),
        vec3(-1.0f,  1.0f,  1.0f),
        vec3(-1.0f, -1.0f,  1.0f),
        vec3( 1.0f, -1.0f,  1.0f),
        vec3( 1.0f,  1.0f,  1.0f),
        vec3( 1.0f, -1.0f, -1.0f),
        vec3( 1.0f,  1.0f, -1.0f),
        vec3( 1.0f, -1.0f, -1.0f),
        vec3( 1.0f,  1.0f,  1.0f),
        vec3( 1.0f, -1.0f,  1.0f),
        vec3( 1.0f,  1.0f,  1.0f),
        vec3( 1.0f,  1.0f, -1.0f),
        vec3(-1.0f,  1.0f, -1.0f),
        vec3( 1.0f,  1.0f,  1.0f),
        vec3(-1.0f,  1.0f, -1.0f),
        vec3(-1.0f,  1.0f,  1.0f),
        vec3( 1.0f,  1.0f,  1.0f),
        vec3(-1.0f,  1.0f,  1.0f),
        vec3( 1.0f, -1.0f,  1.0f)
    }; 

    vec3 position = vertices[gl_VertexIndex];

    out_WorldPosition = position * 2.0f - 1.0f;

    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    mat4 noTranslationView  = perFrameBuffer.View;
    noTranslationView[3]    = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vec4 finalPosition      = perFrameBuffer.Projection * noTranslationView * vec4(out_WorldPosition, 1.0f);
	gl_Position 	        = finalPosition.xyww;
}