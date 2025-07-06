struct PSQuadIn {
    float2 texcoord : TEXCOORD;
    float4 position : SV_POSITION;
};

[[vk::binding(0, 0)]] Texture2D tex : register(t0);
[[vk::binding(0, 1)]] SamplerState sampler_tex : register(s0);

float4 main(PSQuadIn ps_in) : SV_TARGET0
{
    float4 color = tex.Sample(sampler_tex, ps_in.texcoord);
    return color;
}
