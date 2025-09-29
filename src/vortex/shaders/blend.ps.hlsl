struct PSQuadIn {
    float2 texcoord : TEXCOORD;
    float4 position : SV_POSITION;
};

// Textures
[[vk::binding(0, 0)]] Texture2D texA : register(t0);  // Input A (base)
[[vk::binding(0, 1)]] Texture2D texB : register(t1);  // Input B (overlay)
[[vk::binding(0, 2)]] SamplerState sampler_tex : register(s0);

// Constants
cbuffer BlendConstants : register(b0) {
    float4 blend_factor;    // Channel mask + blend factor from OMSetBlendFactor
    float opacity;          // Overall opacity
    int clamp_result;       // Whether to clamp output to [0,1]
    int blend_mode;         // Blend mode enum value
};

float4 ApplyBlendMode(float4 base, float4 overlay, int mode) {
    switch(mode) {
        case 0: // Normal
            return lerp(base, overlay, opacity);
            
        case 1: // Multiply
            return base * overlay;
            
        case 2: // Screen
            return 1.0 - (1.0 - base) * (1.0 - overlay);
            
        case 3: // Overlay
            return base.rgb < 0.5 
                ? 2.0 * base * overlay
                : 1.0 - 2.0 * (1.0 - base) * (1.0 - overlay);
                
        case 4: // Add
            return base + overlay;
            
        case 5: // Subtract
            return base - overlay;
            
        case 6: // Difference
            return abs(base - overlay);
            
        case 7: // Darken
            return min(base, overlay);
            
        case 8: // Lighten
            return max(base, overlay);
            
        default:
            return base;
    }
}

float4 main(PSQuadIn ps_in) : SV_TARGET0 {
    float4 colorA = texA.Sample(sampler_tex, ps_in.texcoord);
    float4 colorB = texB.Sample(sampler_tex, ps_in.texcoord);
    
    // Apply blend mode
    float4 result = ApplyBlendMode(colorA, colorB, blend_mode);
    
    // Apply opacity
    result = lerp(colorA, result, opacity);
    
    // Apply channel masking via blend factor (done by hardware)
    // The blend_factor is used by OMSetBlendFactor for channel masking
    
    // Clamp if requested
    if (clamp_result) {
        result = saturate(result);
    }
    
    return result;
}