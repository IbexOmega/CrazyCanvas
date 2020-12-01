layout(binding = 0, set = DRAW_SET_INDEX) restrict buffer b_Vertices    { SVertex val[]; }  b_Vertices;

layout(location = 0) out float out_PosW;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_Tangent;
layout(location = 3) out vec4 out_TexCoord;

void main()
{
    SVertex vertex = b_Vertices.val[gl_VertexIndex]; 
    out_PosW = vertex.Position.w;
    out_Normal = vertex.Normal;
    out_Tangent = vertex.Tanget;
    out_TexCoord = vertex.TexCoord;
    gl_Position = vec4(b_Vertices.val[gl_VertexIndex].Position.xyz, 1.f);
}