// Color Correction Pixel Shader
// Supports both 1D and 3D LUTs with hardware and manual interpolation
// Also supports brightness, contrast, and saturation adjustments

struct PSQuadIn
{
    float2 texcoord : TEXCOORD;
    float4 position : SV_POSITION;
};

enum LUTInterp
{
    LUTInterp_Nearest,
    LUTInterp_Trilinear,
    LUTInterp_Tetrahedral,
};

struct ColorSettings
{
    uint interp_method; // LUTInterp enum
    float brightness;
    float contrast;
    float saturation;
};

[[vk::push_constant]] ConstantBuffer<ColorSettings> settings : register(b0);
[[vk::binding(0, 0)]] Texture3D lut3d : register(t0);
[[vk::binding(1, 0)]] Texture2D tex : register(t1);
[[vk::binding(0, 1)]] SamplerState sampler_lut : register(s0);
[[vk::binding(1, 1)]] SamplerState sampler_tex : register(s1);

// 1D LUT with manual linear interpolation
float3 Lut1DLinear(const float3 color)
{
    float3 dims;
    lut3d.GetDimensions(dims.x, dims.y, dims.z);
    
    // For 1D LUT stored as 2D texture (width = size, height = 1)
    float lut_size = dims.x;
    
    float3 result;
    for (int i = 0; i < 3; i++)
    {
        float value = saturate(color[i]);
        float scaled = value * (lut_size - 1.0);
        float index = floor(scaled);
        float frac = scaled - index;
        
        // Sample two adjacent LUT entries
        float u0 = (index + 0.5) / lut_size;
        float u1 = (index + 1.5) / lut_size;
        
        float4 c0 = lut3d.Sample(sampler_lut, float3(u0, 0.5, 0.5));
        float4 c1 = lut3d.Sample(sampler_lut, float3(u1, 0.5, 0.5));
        
        result[i] = lerp(c0[i], c1[i], frac);
    }
    
    return result;
}
float3 Lut1DNearest(const float3 color)
{
    float3 dims;
    lut3d.GetDimensions(dims.x, dims.y, dims.z);
    
    // For 1D LUT stored as 2D texture (width = size, height = 1)
    float lut_size = dims.x;
    
    float3 result;
    for (int i = 0; i < 3; i++)
    {
        float value = saturate(color[i]);
        float scaled = value * (lut_size - 1.0);
        float index = round(scaled);
        
        // Sample nearest LUT entry
        float u = (index + 0.5) / lut_size;
        
        float4 c = lut3d.Sample(sampler_lut, float3(u, 0.5, 0.5));
        
        result[i] = c[i];
    }
    
    return result;
}

// 3D LUT with tetrahedral interpolation (manual)
float3 Lut3DTetra(const float3 color)
{
    float3 dims;
    lut3d.GetDimensions(dims.x, dims.y, dims.z);

    // Scale color to LUT space
    float3 re_dims = dims - float3(1.0, 1.0, 1.0);
    float3 restored = saturate(color) * re_dims;

    float3 black = floor(restored);
    float3 white = ceil(restored);
    float3 fracts = frac(restored);

    // Normalize to texture coordinates [0,1]
    float3 blackf = black / re_dims;
    float3 whitef = white / re_dims;

    // Tetrahedral interpolation - select which tetrahedron we're in
    bool3 cmp = fracts.rgb >= fracts.gbr; // (r>g, g>b, b>r)
    int res = min(int(cmp.x) * 4 + int(cmp.y) * 2 + int(cmp.z) - 1, 5);

    // Tetrahedron vertices (6 possible orientations)
    float3 swizzle_rgb[6];
    swizzle_rgb[0] = float3(blackf.x, blackf.y, whitef.z);
    swizzle_rgb[1] = float3(blackf.x, whitef.y, blackf.z);
    swizzle_rgb[2] = float3(blackf.x, whitef.y, blackf.z);
    swizzle_rgb[3] = float3(whitef.x, blackf.y, blackf.z);
    swizzle_rgb[4] = float3(blackf.x, blackf.y, whitef.z);
    swizzle_rgb[5] = float3(whitef.x, blackf.y, blackf.z);

    float3 swizzle_cmy[6];
    swizzle_cmy[0] = float3(blackf.x, whitef.y, whitef.z);
    swizzle_cmy[1] = float3(whitef.x, whitef.y, blackf.z);
    swizzle_cmy[2] = float3(blackf.x, whitef.y, whitef.z);
    swizzle_cmy[3] = float3(whitef.x, blackf.y, whitef.z);
    swizzle_cmy[4] = float3(whitef.x, blackf.y, whitef.z);
    swizzle_cmy[5] = float3(whitef.x, whitef.y, blackf.z);

    float3 swizzle_xyz[6];
    swizzle_xyz[0] = float3(fracts.z, fracts.y, fracts.x);
    swizzle_xyz[1] = float3(fracts.y, fracts.x, fracts.z);
    swizzle_xyz[2] = float3(fracts.y, fracts.z, fracts.x);
    swizzle_xyz[3] = float3(fracts.x, fracts.z, fracts.y);
    swizzle_xyz[4] = float3(fracts.z, fracts.x, fracts.y);
    swizzle_xyz[5] = float3(fracts.x, fracts.y, fracts.z);

    float3 point_rgb = swizzle_rgb[res];
    float3 point_cmy = swizzle_cmy[res];
    float3 delta = swizzle_xyz[res];

    // Sample the 4 corners of the tetrahedron
    float4 s1 = lut3d.Sample(sampler_lut, blackf);
    float4 s2 = lut3d.Sample(sampler_lut, point_rgb);
    float4 s3 = lut3d.Sample(sampler_lut, point_cmy);
    float4 s4 = lut3d.Sample(sampler_lut, whitef);

    // Tetrahedral interpolation
    return ((1.0 - delta.x) * s1 + (delta.x - delta.y) * s2 + (delta.y - delta.z) * s3 + delta.z * s4).rgb;
}

// 3D LUT with trilinear interpolation or point sampling (hardware)
float3 Lut3DSampled(const float3 color)
{
    // Hardware trilinear filtering handles interpolation
    return lut3d.Sample(sampler_lut, saturate(color)).rgb;
}

// Apply brightness, contrast, and saturation adjustments
float3 ApplyColorAdjustments(float3 color)
{
    // Apply brightness
    color += settings.brightness;
    
    // Apply contrast (around 0.5 midpoint)
    color = (color - 0.5) * settings.contrast + 0.5;
    
    // Apply saturation
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722)); // Rec. 709 luma coefficients
    color = lerp(float3(luminance, luminance, luminance), color, settings.saturation);
    
    return color;
}

float4 main(PSQuadIn ps_in) : SV_TARGET0
{
    float4 color = tex.Sample(sampler_tex, ps_in.texcoord);
    
    // Apply basic color adjustments first
    float3 adjusted_color = ApplyColorAdjustments(color.rgb);
    
    // Detect LUT type based on texture dimensions
    float3 dims;
    lut3d.GetDimensions(dims.x, dims.y, dims.z);
    
    float3 result_color = adjusted_color;
    
    // Only apply LUT if dimensions suggest it's a valid LUT texture
    // (Skip if dimensions are same as input texture, which means no LUT was loaded)
    if (dims.x >= 16) // Minimum LUT size check
    {
        if (dims.y == 1 && dims.z == 1)
        {
            // 1D LUT (stored as 3D texture with height=1, depth=1)
            result_color = (settings.interp_method == LUTInterp_Nearest) ?
                Lut1DNearest(adjusted_color) : Lut1DLinear(adjusted_color);
        }
        else if (dims.y > 1 && dims.z > 1)
        {
            // 3D LUT
            result_color = (settings.interp_method == LUTInterp_Tetrahedral) ?
                Lut3DTetra(adjusted_color) : Lut3DSampled(adjusted_color);
        }
    }
    
    return float4(result_color, color.a);
}
