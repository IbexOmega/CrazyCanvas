#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer   { SPerFrameBuffer val; }      u_PerFrameBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_Albedo;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_AORoughMetal;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_CompactNormals;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_Velocity;
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_DepthStencil;

layout(location = 0) out vec4	out_Color;

void main()
{
    vec4 sampledAlbedo      = texture(u_Albedo, in_TexCoord);
    out_Color               = vec4(sampledAlbedo.rgb, 1.0f);
}