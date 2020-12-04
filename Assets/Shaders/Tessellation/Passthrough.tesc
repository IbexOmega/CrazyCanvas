#version 460

layout (vertices = 3) out;

layout(binding = 1, set = 0) restrict readonly buffer CalculationData				
{
    mat4 ScaleMatrix;
    uint PrimitiveCounter;
    vec3 Padding;
}	b_CalculationData;

layout(location = 0) in float in_PosW[];
layout(location = 1) in vec4 in_Normal[];
layout(location = 2) in vec4 in_Tangent[];
layout(location = 3) in vec4 in_TexCoord[];

layout(location = 0) out float out_PosW[3];
layout(location = 1) out vec4 out_Normal[3];
layout(location = 2) out vec4 out_Tangent[3];
layout(location = 3) out vec4 out_TexCoord[3];


float CalculateTrisSurfaceAreaSquared()
{
    mat4 scaleMatrix = b_CalculationData.ScaleMatrix;
    
    vec3 e0 = (gl_in[1].gl_Position -  gl_in[0].gl_Position).xyz;
    vec3 e1 = (gl_in[2].gl_Position -  gl_in[0].gl_Position).xyz;
    e0 = (scaleMatrix * vec4(e0, 1.0f)).xyz;
    e1 = (scaleMatrix * vec4(e1, 1.0f)).xyz;

    // Naive
    float area = length(e0) * length(e1) * 0.5f;
    // Smart
    // vec3 crossVector = cross(e0, e1);
    // float areaSquared = dot(crossVector, crossVector) * 0.5f;

    return area;
}

void main(void)
{
    const float MAX_AREA = 50.0f;
    float surfaceAreaSqrt = CalculateTrisSurfaceAreaSquared();
    
    float t = min(surfaceAreaSqrt / MAX_AREA, 1.0f);
    float innerTessLevel = mix(1.0, 8.0, t);
    float outerTessLevel = mix(1.0, 4.0, t);

    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = innerTessLevel;
        gl_TessLevelOuter[0] = outerTessLevel;
        gl_TessLevelOuter[1] = outerTessLevel;
        gl_TessLevelOuter[2] = outerTessLevel;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	out_PosW[gl_InvocationID] = in_PosW[gl_InvocationID];
	out_Normal[gl_InvocationID] = in_Normal[gl_InvocationID];	
	out_Tangent[gl_InvocationID] = in_Tangent[gl_InvocationID];	
	out_TexCoord[gl_InvocationID] = in_TexCoord[gl_InvocationID];
}