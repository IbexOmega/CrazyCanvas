#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable

in vec2 attr_pos;

#ifdef HAS_COLOR
    in vec4 attr_color;
    out vec4 color;
#endif

#ifdef HAS_UV0
    in vec2 attr_tex0;
    out vec2 uv0;
#endif

#ifdef HAS_UV1
    in vec2 attr_tex1;
    out vec2 uv1;
#endif

#ifdef HAS_UV2
    in vec4 attr_tex2;
    out vec4 uv2;
#endif

#ifdef HAS_ST1
    out vec2 st1;
#endif

#ifdef HAS_COVERAGE
    in float attr_coverage;
    out float coverage;
#endif

uniform mat4 projectionMtx;
uniform vec2 textSize;

////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    gl_Position = vec4(attr_pos, 0, 1) * projectionMtx;

#ifdef HAS_COLOR
    color = attr_color;
#endif

#ifdef HAS_UV0
    uv0 = attr_tex0;
#endif

#ifdef HAS_UV1
    uv1 = attr_tex1;
#endif

#ifdef HAS_UV2
    uv2 = attr_tex2;
#endif

#ifdef HAS_ST1
    st1 = attr_tex1 * textSize.xy;
#endif

#ifdef HAS_COVERAGE
    coverage = attr_coverage;
#endif
}