// Transform pixel shader
// Applies crop rectangle and samples the texture

struct CropConstants
{
    float4 crop_rect; // x, y, width, height (normalized 0-1)
};

[[vk::push_constant]] ConstantBuffer<CropConstants> crop : register(b0);

struct PSQuadIn
{
    float2 texcoord : TEXCOORD;
    float4 position : SV_POSITION;
};

[[vk::binding(0, 0)]] Texture2D tex_b : register(t0);
[[vk::binding(0, 1)]] SamplerState sampler_tex : register(s0);

bool IsPointInRectangleExclusive(float2 xpoint, float2 rectMin, float2 rectMax)
{
    return all(and(xpoint > rectMin, xpoint < rectMax));
}

float4 main(PSQuadIn ps_in) : SV_TARGET0
{
    // Apply crop rectangle
    // if uv in crop rect -> sample texture, else return transparent
    
    return IsPointInRectangleExclusive(ps_in.texcoord, crop.crop_rect.xy, crop.crop_rect.xy + crop.crop_rect.zw) ?
        tex_b.Sample(sampler_tex, ps_in.texcoord) :
        float4(0.0f, 0.0f, 0.0f, 0.0f);
}
