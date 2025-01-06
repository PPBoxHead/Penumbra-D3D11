cbuffer ConstantBuffer : register(b0)
{
    matrix worldMatrix;
};

struct VertexIn
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut vs_main(VertexIn input)
{
    VertexOut output;
    
    output.Position = mul(float4(input.Position, 1.0f), worldMatrix);
    output.Color = input.Color;
    
    return output;
}