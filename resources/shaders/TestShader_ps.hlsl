struct PixelInput {
    float4 position : SV_POSITION;
    float4 color : COL;
};

struct PixelOutput {
    float4 attachment0 : SV_TARGET0;
};

PixelOutput ps_main(PixelInput input) : SV_TARGET {
    PixelOutput output;
    output.attachment0 = input.color;
    return output;
}