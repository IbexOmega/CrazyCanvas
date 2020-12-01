#version 450

layout (vertices = 3) out;

layout(location = 0) in float in_PosW[];
layout(location = 1) in vec4 in_Normal[];
layout(location = 2) in vec4 in_Tangent[];
layout(location = 3) in vec4 in_TexCoord[];

layout(location = 0) out float out_PosW[3];
layout(location = 1) out vec4 out_Normal[3];
layout(location = 2) out vec4 out_Tangent[3];
layout(location = 3) out vec4 out_TexCoord[3];

void main(void)
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	out_PosW[gl_InvocationID] = in_PosW[gl_InvocationID];
	out_Normal[gl_InvocationID] = in_Normal[gl_InvocationID];	
	out_Tangent[gl_InvocationID] = in_Tangent[gl_InvocationID];	
	out_TexCoord[gl_InvocationID] = in_TexCoord[gl_InvocationID];	
}