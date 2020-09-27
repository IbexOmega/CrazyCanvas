#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "NoesisInclude.glsl"

struct SNoesisVertex
{
    vec2 attr_pos;

#ifdef HAS_COLOR
    vec4 attr_color;
#endif

#ifdef HAS_UV0
    vec2 attr_tex0;
#endif

#ifdef HAS_UV1
    vec2 attr_tex1;
#endif

#ifdef HAS_UV2
    vec4 attr_tex2;
#endif

#ifdef HAS_COVERAGE
    float attr_coverage;
#endif
};

#ifdef HAS_COLOR
    layout(location = 0) out vec4 color;
#endif

#ifdef HAS_UV0
    layout(location = 1) out vec2 uv0;
#endif

#ifdef HAS_UV1
    layout(location = 2) out vec2 uv1;
#endif

#ifdef HAS_UV2
    layout(location = 3) out vec4 uv2;
#endif

#ifdef HAS_ST1
    layout(location = 4) out vec2 st1;
#endif

#ifdef HAS_COVERAGE
    layout(location = 5) out float coverage;
#endif

layout(binding = 0, set = 0) restrict readonly buffer   Vertices    { SNoesisVertex v[]; }  b_Vertices;
layout(binding = 1, set = 0) uniform                    Params      { SNoesisParams v; }    u_Params;

////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    SNoesisVertex vertex = b_Vertices.v[gl_VertexIndex];
    SNoesisParams params = u_Params.v;
    gl_Position = vec4(vertex.attr_pos, 0, 1) * params.ProjMatrix;

#ifdef HAS_COLOR
    color = vertex.attr_color;
#endif

#ifdef HAS_UV0
    uv0 = vertex.attr_tex0;
#endif

#ifdef HAS_UV1
    uv1 = vertex.attr_tex1;
#endif

#ifdef HAS_UV2
    uv2 = vertex.attr_tex2;
#endif

#ifdef HAS_ST1
    st1 = vertex.attr_tex1 * params.TextSize.xy;
#endif

#ifdef HAS_COVERAGE
    coverage = vertex.attr_coverage;
#endif
}