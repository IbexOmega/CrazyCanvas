#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Defines.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer              { SPerFrameBuffer val; }    u_PerFrameBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_UnwrappedTexture;

layout(location = 0) out vec4 out_MaskTexture;

void main()
{
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    vec3 worldPosition = texture(u_UnwrappedTexture, in_TexCoord).xyz;
    vec4 clipPosition = perFrameBuffer.Projection * perFrameBuffer.View * vec4(worldPosition, 1.f);
    float depth = clipPosition.z;
    vec3 cameraDirection = normalize(vec3(-perFrameBuffer.View[0][2], -perFrameBuffer.View[1][2], -perFrameBuffer.View[2][2]));
    vec3 targetPosition = perFrameBuffer.CameraPosition.xyz;
    vec3 projectedPosition = targetPosition + cameraDirection*depth;
    const float BRUSH_SIZE = 0.5f;
    
    if(length(worldPosition-projectedPosition) <= BRUSH_SIZE)
        out_MaskTexture = vec4(1.f, 1.f, 1.f, 1.f);
    else
        out_MaskTexture = vec4(worldPosition, 1.f);
}
