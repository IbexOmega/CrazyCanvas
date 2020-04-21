#version 450
#extension GL_ARB_separate_shader_objects : enable

struct SVertex
{
    vec4    Position;
    vec4    Normal;
    vec4    Tangent;
    vec4    TexCoord;
};

struct SInstance
{
    mat4    Transform;
    uint    Mask_MeshMaterialIndex;
    uint    Flags_SBTRecordOffset;
    uint    AccelerationStructureHandleTop32;
    uint    AccelerationStructureHandleBottom32;
};

struct SMeshIndexDesc
{
    uint	IndexCount;
    uint	InstanceCount;
    uint	FirstIndex;
    int	    VertexOffset;
    uint	FirstInstance;
    
    uint	MaterialIndex;
    uint	Padding0;
    uint    Padding1;
};

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 Position;
	vec4 Right;
	vec4 Up;
};

struct SMaterialParameters
{
    vec4    Albedo;
    float   Ambient;
    float   Metallic;
    float   Roughness;
    float   Unreserved;
};

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0 , set = 0) uniform sampler2D u_VisibilityBuffer;
layout(binding = 1 , set = 0) buffer Vertices            { SVertex val[]; }              b_Vertices;
layout(binding = 2 , set = 0) buffer Indices             { uint val[]; }                 b_Indices;
layout(binding = 3 , set = 0) buffer Instances           { SInstance val[]; }            b_Instances;
layout(binding = 4 , set = 0) buffer MeshIndices         { SMeshIndexDesc val[]; }       b_MeshIndices;
layout(binding = 5 , set = 0) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;
layout(binding = 6 , set = 0) buffer MaterialParameters  { SMaterialParameters val[]; }  b_MaterialParameters;
layout(binding = 7 , set = 0) uniform sampler2D u_SceneAlbedoMaps[32];
layout(binding = 8 , set = 0) uniform sampler2D u_SceneNormalMaps[32];
layout(binding = 9 , set = 0) uniform sampler2D u_SceneAOMaps[32];
layout(binding = 10, set = 0) uniform sampler2D u_SceneMetallicMaps[32];
layout(binding = 11, set = 0) uniform sampler2D u_SceneRougnessMaps[32];

layout(location = 0) out vec4	out_Color;

struct InterpolatedVertex
{
    vec3    Position;
    vec3    Normal;
    vec3    Tangent;
    vec2    TexCoord;
};

struct DerivativesOutput
{
	vec3 dbDx;
	vec3 dbDy;
};

DerivativesOutput  ComputePartialDerivatives(vec2 v[3])
{
    DerivativesOutput derivatives;
	float d = 1.0f / determinant(mat2(v[2] - v[1], v[0] - v[1]));
	derivatives.dbDx = vec3(v[1].y - v[2].y, v[2].y - v[0].y, v[0].y - v[1].y) * d;
	derivatives.dbDy = vec3(v[2].x - v[1].x, v[0].x - v[2].x, v[1].x - v[0].x) * d;
    return derivatives;
}

// Interpolate 2D attributes using the partial derivatives and generates dx and dy for texture sampling.
vec2 Interpolate2DAttributes(mat3x2 attributes, vec3 dbDx, vec3 dbDy, vec2 d)
{
	vec3 attr0 = vec3(attributes[0].x, attributes[1].x, attributes[2].x);
	vec3 attr1 = vec3(attributes[0].y, attributes[1].y, attributes[2].y);
	vec2 attribute_x = vec2(dot(dbDx,attr0), dot(dbDx,attr1));
	vec2 attribute_y = vec2(dot(dbDy,attr0), dot(dbDy,attr1));
	vec2 attribute_s = attributes[0];
	
	vec2 result = (attribute_s + d.x * attribute_x + d.y * attribute_y);
	return result;
}

// Interpolate vertex attributes at point 'd' using the partial derivatives
vec3 Interpolate3DAttributes(mat3 attributes, vec3 dbDx, vec3 dbDy, vec2 d)
{
	vec3 attribute_x = attributes * dbDx;
	vec3 attribute_y = attributes * dbDy;
	vec3 attribute_s = attributes[0];
	
	return (attribute_s + d.x * attribute_x + d.y * attribute_y);
}

InterpolatedVertex LoadVertexData(SMeshIndexDesc meshIndexDesc, SPerFrameBuffer perFrameBuffer, SInstance instance, uint primitiveID)
{
    SVertex[3] vertices;

    uint baseIndex = meshIndexDesc.FirstIndex;
    int baseVertex = meshIndexDesc.VertexOffset;

    uint index0 = b_Indices.val[baseIndex + primitiveID * 3 + 0];
    uint index1 = b_Indices.val[baseIndex + primitiveID * 3 + 1];
    uint index2 = b_Indices.val[baseIndex + primitiveID * 3 + 2];

    SVertex vertex0 = b_Vertices.val[baseVertex + index0];
    SVertex vertex1 = b_Vertices.val[baseVertex + index1];
    SVertex vertex2 = b_Vertices.val[baseVertex + index2];

    mat4 mvp = perFrameBuffer.Projection * perFrameBuffer.View * instance.Transform;

    //Compute Triangle Partial Derivatives for Attribute interpolation
    vec4 clipSpace0 = mvp * vec4(vertex0.Position.xyz, 1.0f);
    vec4 clipSpace1 = mvp * vec4(vertex1.Position.xyz, 1.0f);
    vec4 clipSpace2 = mvp * vec4(vertex2.Position.xyz, 1.0f);

    // Pre-calculate 1 over w components
    vec3 oneOverW = 1.0f / vec3(clipSpace0.w, clipSpace1.w, clipSpace2.w);

    clipSpace0 *= oneOverW[0];
    clipSpace1 *= oneOverW[1];
    clipSpace2 *= oneOverW[2];
    vec2 screenCoords[3] = { clipSpace0.xy, clipSpace1.xy, clipSpace2.xy };

    DerivativesOutput derivatives = ComputePartialDerivatives(screenCoords);

    vec2 screenPos = (in_TexCoord * 2.0f - 1.0f);
    vec2 delta = screenPos - screenCoords[0];

    InterpolatedVertex interpolatedVertex;

    // Interpolate Position
    mat3 triPosition =
    {
        vertex0.Position.xyz,
        vertex1.Position.xyz,
        vertex2.Position.xyz
    };

    interpolatedVertex.Position = vec3((instance.Transform * vec4(Interpolate3DAttributes(triPosition, derivatives.dbDx, derivatives.dbDy, delta), 1.0f)).xyz);

    // Interpolate Normal
    mat3 triNormals =
    {
        vertex0.Normal.xyz,
        vertex1.Normal.xyz,
        vertex2.Normal.xyz
    };

    interpolatedVertex.Normal = vec3((instance.Transform * vec4(Interpolate3DAttributes(triNormals, derivatives.dbDx, derivatives.dbDy, delta), 0.0f)).xyz);

    // Interpolate Tangent
    mat3 triTangents =
    {
        vertex0.Tangent.xyz,
        vertex1.Tangent.xyz,
        vertex2.Tangent.xyz
    };

    interpolatedVertex.Tangent = vec3((instance.Transform * vec4(Interpolate3DAttributes(triTangents, derivatives.dbDx, derivatives.dbDy, delta), 0.0f)).xyz);

    // Interpolate TexCoords
    mat3x2 triTexCoords =
    {
        vertex0.TexCoord.xy,
        vertex1.TexCoord.xy,
        vertex2.TexCoord.xy
    };

    interpolatedVertex.TexCoord = Interpolate2DAttributes(triTexCoords, derivatives.dbDx, derivatives.dbDy, delta);

    return interpolatedVertex;
}

void main()
{
    vec4 visibilityRaw          = texture(u_VisibilityBuffer, in_TexCoord);

    if (visibilityRaw != vec4(0.0f))
    {
        uint primID_instanceID          = packUnorm4x8(visibilityRaw);	
        uint primitiveID                = ((primID_instanceID >> 10) & 0x003FFFFF) - 1;
		uint instanceID                 = (primID_instanceID & 0x000003FF);

        SInstance instance              = b_Instances.val[instanceID];

        uint meshIndexID                = (instance.Mask_MeshMaterialIndex) & 0x00FFFFFF;
        SMeshIndexDesc meshIndexDesc    = b_MeshIndices.val[meshIndexID];

        SPerFrameBuffer perFrameBuffer  = u_PerFrameBuffer.val;
        
        InterpolatedVertex vertex = LoadVertexData(meshIndexDesc, perFrameBuffer, instance, primitiveID);

        out_Color = texture(u_SceneAlbedoMaps[meshIndexDesc.MaterialIndex], vertex.TexCoord);
        //out_Color = vec4(vertex.TexCoord, 0.0f, 1.0f);
        return;
    }

    out_Color = vec4(1.0f, 0.0f, 1.0f, 1.0f);
}