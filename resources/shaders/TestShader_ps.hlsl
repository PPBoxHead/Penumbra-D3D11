struct PixelInput {
    float4 color : COLOR;
};

struct PixelOutput {
    float4 attachment0 : SV_TARGET0;
};


PixelOutput ps_main(PixelInput input) : SV_TARGET {
    float4 inColor = input.color;
    
    PixelOutput output;
    output.attachment0 = inColor;
    return output;
}