struct PixelInput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

struct PixelOutput
{
    float4 attachment0 : SV_TARGET0;
};

PixelOutput ps_main(PixelInput input) : SV_TARGET
{
    PixelOutput output;
    output.attachment0 = input.Color;
    return output;
}