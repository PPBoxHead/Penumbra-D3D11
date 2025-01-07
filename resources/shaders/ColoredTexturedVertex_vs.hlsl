struct VertexIn
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
    matrix worldMatrix;
};

VertexOut vs_main(VertexIn input)
{
    VertexOut output;
    
    output.Position = mul(float4(input.Position, 1.0f), worldMatrix);
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    
    return output;
}