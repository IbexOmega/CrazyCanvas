#version 450

layout (triangles, fractional_odd_spacing, cw) in;

layout(location = 0) in float in_PosW[];
layout(location = 1) in vec4 in_Normal[];
layout(location = 2) in vec4 in_Tangent[];
layout(location = 3) in vec4 in_TexCoord[];

layout(location = 0) out float out_PosW;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_Tangent;
layout(location = 3) out vec4 out_TexCoord;

void main(void)
{
    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[2].gl_Position);
	
	out_PosW = gl_TessCoord.x*in_PosW[0] + gl_TessCoord.y*in_PosW[1] + gl_TessCoord.z*in_PosW[2];
	out_Normal = gl_TessCoord.x*in_Normal[0] + gl_TessCoord.y*in_Normal[1] + gl_TessCoord.z*in_Normal[2];
	out_Tangent = gl_TessCoord.x*in_Tangent[0] + gl_TessCoord.y*in_Tangent[1] + gl_TessCoord.z*in_Tangent[2];
	out_TexCoord = gl_TessCoord.x*in_TexCoord[0] + gl_TessCoord.y*in_TexCoord[1] + gl_TessCoord.z*in_TexCoord[2];
}