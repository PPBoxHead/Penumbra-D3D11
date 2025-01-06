struct PixelInput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

Texture2D texture_input : register(t0); // Texture sampler
SamplerState samplerState : register(s0); // Sampler state

float4 ps_main(PixelInput input) : SV_TARGET
{
    // Sample the texture and multiply by vertex color
    float4 textureColor = texture_input.Sample(samplerState, input.TexCoord);
    return textureColor * input.Color;
}
