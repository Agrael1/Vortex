// Compute shader to convert RGBA8 to UYVY format for NDI
// UYVY is a 4:2:2 packed YUV format where each pair of pixels shares UV values
// Format: U0 Y0 V0 Y1 (for 2 pixels)

[[vk::binding(0, 0)]] Texture2D<float4> rgbaInput : register(t0);
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> uyvyOutput : register(u0);

// BT.709 RGB to YUV conversion matrix (for HD content)
// Y  =  0.2126*R + 0.7152*G + 0.0722*B
// U  = -0.1146*R - 0.3854*G + 0.5000*B + 128
// V  =  0.5000*R - 0.4542*G - 0.0458*B + 128

float3 RGBtoYUV(float3 rgb)
{
    float3 yuv;
    yuv.x = dot(rgb, float3(0.2126, 0.7152, 0.0722)); // Y
    yuv.y = dot(rgb, float3(-0.1146, -0.3854, 0.5000)) + 0.5; // U (normalized to [0,1])
    yuv.z = dot(rgb, float3(0.5000, -0.4542, -0.0458)) + 0.5; // V (normalized to [0,1])
    
    // Transform YUV to video range
    yuv.x = yuv.x * (235.0 / 255.0) + (16.0 / 255.0); // Y in [16/255, 235/255]
    yuv.y = yuv.y * (224.0 / 255.0) + (16.0 / 255.0); // U in [16/255, 240/255]
    yuv.z = yuv.z * (224.0 / 255.0) + (16.0 / 255.0); // V in [16/255, 240/255]
    
    return yuv;
}

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // Each thread processes 2 horizontal pixels to create one UYVY quad
    uint2 pixelPos = dispatchThreadID.xy * uint2(2, 1);
    
    // Get dimensions
    uint width, height;
    rgbaInput.GetDimensions(width, height);
    
    // Bounds check
    if (pixelPos.x >= width || pixelPos.y >= height)
        return;
    
    // Read two adjacent pixels
    float4 rgba0 = rgbaInput[pixelPos];
    float4 rgba1 = (pixelPos.x + 1 < width) ? rgbaInput[pixelPos + uint2(1, 0)] : rgba0;
    
    
    // Convert to YUV
    float3 yuv0 = RGBtoYUV(rgba0.rgb);
    float3 yuv1 = RGBtoYUV(rgba1.rgb);
    
    // Average the U and V components (4:2:2 subsampling)
    float u = (yuv0.y + yuv1.y) * 0.5;
    float v = (yuv0.z + yuv1.z) * 0.5;
    
    // Pack as UYVY: U Y0 V Y1
    // Convert to 8-bit values [0, 255]
    uint4 uyvy = uint4(
        uint(u * 255.0 + 0.5),
        uint(yuv0.x * 255.0 + 0.5),
        uint(v * 255.0 + 0.5),
        uint(yuv1.x * 255.0 + 0.5)
    );
    
    // Calculate buffer offset
    // Each output element is one uint (4 bytes) containing UYVY for 2 pixels
    // Row stride is (width / 2) since each element covers 2 pixels
    uint outputX = pixelPos.x / 2;
    uint outputY = pixelPos.y;
    uint rowStride = width / 2;
    uint offset = outputY * rowStride + outputX;
    
    // Pack into uint (little endian: U Y0 V Y1)
    uint packed = (uyvy.w << 24) | (uyvy.z << 16) | (uyvy.y << 8) | uyvy.x;
    
    // Write to buffer
    uyvyOutput[offset] = packed;
}
