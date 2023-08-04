// VS_IN
struct CommonShaderVertex
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 Bitangent : BITANGENT;
    float2 UV : TEXCOORD0;
};

#define TEXTUREFLAGS_NONE 1
#define TEXTUREFLAGS_DIFFUSE 1
#define TEXTUREFLAGS_NORMAL 2