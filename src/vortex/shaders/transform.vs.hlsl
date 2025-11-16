// Transform vertex shader
// Applies 2D transformations: translation, rotation (as 2D rotation), scale, and pivot point

struct TransformConstants
{
    float2 translation;   // Translation in normalized coordinates
    float2 scale;         // Scale factors
    float2 pivot;         // Pivot point (normalized 0-1)
    float rotation;       // Rotation angle in radians
    float aspect_ratio;   // Width / Height of the output
};

[[vk::push_constant]] ConstantBuffer<TransformConstants> transform : register(b0);

struct VSQuadOut
{
    float2 texcoord : TexCoord;
    float4 position : SV_Position;
};

VSQuadOut main(uint VertexID : SV_VertexID)
{
    VSQuadOut Out;
    
    // Generate full screen quad positions (-1 to 1)
    Out.texcoord = float2((VertexID << 1) & 2, VertexID & 2);
    float2 position = Out.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    
    // Apply transformations in object space (before NDC conversion)
    // Convert from NDC [-1,1] to [0,1] for easier transformation
    float2 pos01 = position * 0.5f + 0.5f;
    
    // Translate pivot point to origin
    pos01 -= transform.pivot;
    
    // Correct for aspect ratio before rotation
    pos01.x *= transform.aspect_ratio;
    
    // Apply scale
    pos01 *= transform.scale;
    
    // Apply 2D rotation
    float cosR = cos(transform.rotation);
    float sinR = sin(transform.rotation);
    float2 rotated;
    rotated.x = pos01.x * cosR - pos01.y * sinR;
    rotated.y = pos01.x * sinR + pos01.y * cosR;
    
    // Undo aspect ratio correction after rotation
    rotated.x /= transform.aspect_ratio;
    
    // Translate pivot point back and apply translation
    pos01 = rotated + transform.pivot + transform.translation;
    
    // Convert back to NDC [-1,1]
    position = pos01 * 2.0f - 1.0f;
    
    Out.position = float4(position, 0.0f, 1.0f);
    return Out;
}
