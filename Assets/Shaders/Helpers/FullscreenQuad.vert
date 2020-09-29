#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_debug_printf : enable

layout(location = 0) out vec2 out_TexCoord;

void main() 
{
	vec2 texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	vec2 position = (texCoord * 2.0f) - 1.0f;
	position.y = -position.y;
	
    out_TexCoord 	= texCoord;
	gl_Position 	= vec4(position, 0.0, 1.0);
}