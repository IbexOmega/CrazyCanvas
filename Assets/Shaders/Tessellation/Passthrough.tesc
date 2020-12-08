#version 460

layout (vertices = 3) out;

layout(binding = 1, set = 0) restrict buffer CalculationData				
{
    mat4 ScaleMatrix;
    uint PrimitiveCounter;
    float MaxInnerLevelTess;
    float MaxOuterLevelTess;
    float Padding;
}	b_CalculationData;

layout(location = 0) in float in_PosW[];
layout(location = 1) in vec4 in_Normal[];
layout(location = 2) in vec4 in_Tangent[];
layout(location = 3) in vec4 in_TexCoord[];

layout(location = 0) out float out_PosW[3];
layout(location = 1) out vec4 out_Normal[3];
layout(location = 2) out vec4 out_Tangent[3];
layout(location = 3) out vec4 out_TexCoord[3];

void CalculateTessLevels(inout float inner, inout float  outer[3])
{
    const float MAX_AREA = 20.0;
    const float MIN_AREA = 0.1;
    mat4 scaleMatrix = b_CalculationData.ScaleMatrix;
    
    vec3 e0 = (gl_in[1].gl_Position - gl_in[2].gl_Position).xyz;
    vec3 e1 = (gl_in[0].gl_Position - gl_in[1].gl_Position).xyz;
    vec3 e2 = (gl_in[2].gl_Position - gl_in[0].gl_Position).xyz;
    e0 = (scaleMatrix * vec4(e0, 1.0f)).xyz;
    e1 = (scaleMatrix * vec4(e1, 1.0f)).xyz;
    e2 = (scaleMatrix * vec4(e2, 1.0f)).xyz;

    float maxEdgeLength = max(max(length(e0), length(e1)), length(e2));
    float minEdgeLength = min(min(length(e0), length(e1)), length(e2));
    float diff = smoothstep(0.0, 1.0, minEdgeLength / maxEdgeLength);

    // Naive
    float area = length(e0) * length(e1) * 0.5f;
    float tt = min(max(max(area - MIN_AREA, 0.0f) / (MAX_AREA - MIN_AREA), 0.0f), 1.0f);
    
    inner = ceil(mix(1.0, b_CalculationData.MaxInnerLevelTess, tt));
    
    // float t = max(min((edgeLength - MIN_LENGTH), MAX_LENGTH) / MAX_LENGTH, 0.0f) * tt;
    float outerTess = mix(1.0, b_CalculationData.MaxOuterLevelTess, diff * smoothstep(0.0f, 1.0f, tt));

    outer[0] = outerTess;
    outer[1] = outerTess;
    outer[2] = outerTess;
    // float edgeLength = length(e0);
    // float t0 = max(min((edgeLength - MIN_LENGTH), MAX_LENGTH) / MAX_LENGTH, 0.0f);
    // outer[0] = mix(1.0, b_CalculationData.MaxOuterLevelTess, t0);

    // edgeLength = length(e1);
    // float t1 = max(min((edgeLength - MIN_LENGTH), MAX_LENGTH) / MAX_LENGTH, 0.0f);
    // outer[1] = mix(1.0, b_CalculationData.MaxOuterLevelTess, t1);

    // edgeLength = length(e2);
    // float t2 = max(min((edgeLength - MIN_LENGTH), MAX_LENGTH) / MAX_LENGTH, 0.0f);
    // outer[2] = mix(1.0, b_CalculationData.MaxOuterLevelTess, t2);
}

void main(void)
{
    float inner = 1.0f;
    float outer[3] = {1.0f, 1.0f, 1.0f};
    CalculateTessLevels(inner, outer);

    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = inner;
        gl_TessLevelOuter[0] = outer[0];
        gl_TessLevelOuter[1] = outer[1];
        gl_TessLevelOuter[2] = outer[2];
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	out_PosW[gl_InvocationID] = in_PosW[gl_InvocationID];
	out_Normal[gl_InvocationID] = in_Normal[gl_InvocationID];	
	out_Tangent[gl_InvocationID] = in_Tangent[gl_InvocationID];	
	out_TexCoord[gl_InvocationID] = in_TexCoord[gl_InvocationID];
}