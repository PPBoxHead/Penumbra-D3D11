cbuffer cb : register(b0) {
    row_major float4x4 projectionMatrix : packoffset(c0);
    row_major float4x4 modelMatrix : packoffset(c4);
    row_major float4x4 viewMatrix : packoffset(c8);
};

struct VertexIn {
    float3 position : POS;
    float4 color : COL;
};

struct VertexOut {
    float4 position : SV_POSITION;
    float4 color : COL;
};

VertexOut vs_main(VertexIn input) {
    float4 inColor = input.color;
    float3 inPos = input.position;
    float4 position = mul(float4(inPos, 1.0), mul(modelMatrix, mul(viewMatrix, projectionMatrix)));
    
    VertexOut output;
    output.position = position;
    output.color = input.color;
    return output;
}