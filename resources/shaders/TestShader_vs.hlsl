struct VertexIn {
    float3 position : POS;
    float4 color : COL;
};

struct VertexOut {
    float4 position : SV_POSITION;
    float4 color : COL;
};

VertexOut vs_main(VertexIn input) {
    VertexOut output;
    output.position = float4(input.position, 1.0);;
    output.color = input.color;
    return output;
}