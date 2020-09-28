#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "NoesisInclude.glsl"
    
layout(location = 0) in vec2 attr_pos;

#if defined(HAS_COLOR)
    layout(location = 1) in vec4 attr_color;
    layout(location = 0) out vec4 color;
#endif

#if defined(HAS_UV0)
    layout(location = 2) in vec2 attr_tex0;
    layout(location = 1) out vec2 uv0;
#endif

#if defined(HAS_UV1)
    layout(location = 3) in vec2 attr_tex1;
    layout(location = 2) out vec2 uv1;
#endif

#if defined(HAS_UV2)
    layout(location = 4) in vec4 attr_tex2;
    layout(location = 3) out vec4 uv2;
#endif

#if defined(HAS_ST1)
    layout(location = 4) out vec2 st1;
#endif

#if defined(HAS_COVERAGE)
    layout(location = 5) in float attr_coverage;
    layout(location = 5) out float coverage;
#endif

layout(binding = 0, set = 0) uniform Params { SNoesisParams v; } u_Params;

////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    SNoesisParams params = u_Params.v;
    vec4 clipPos = params.ProjMatrix * vec4(attr_pos.x, attr_pos.y, 0, 1);
    gl_Position = vec4(clipPos.x, -clipPos.y, clipPos.zw);

#if defined(HAS_COLOR)
    color = attr_color;
#endif

#if defined(HAS_UV0)
    uv0 = attr_tex0;
#endif

#if defined(HAS_UV1)
    uv1 = attr_tex1;
#endif

#if defined(HAS_UV2)
    uv2 = attr_tex2;
#endif

#if defined(HAS_ST1)
    st1 = attr_tex1 * params.TextSize.xy;
#endif

#if defined(HAS_COVERAGE)
    coverage = attr_coverage;
#endif
}