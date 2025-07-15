#include <vortex/gfx/texture.h>
#include <vortex/graphics.h>
#include <vortex/util/log.h>

wis::RenderTarget vortex::Texture2D::CreateRenderTarget(const vortex::Graphics& gfx) const noexcept
{
    if (!_texture) {
        vortex::error("Texture::CreateRenderTarget: Texture is not initialized.");
        return {}; // Return an empty render target if texture is not initialized
    }

    wis::Result result = wis::success;
    // Create a render target view for the texture
    wis::RenderTarget rt = gfx.GetDevice().CreateRenderTarget(result, _texture, wis::RenderTargetDesc{
                                                                                        .format = _format,
                                                                                });
    if (!success(result)) {
        vortex::error("Texture::CreateRenderTarget: Failed to create render target view: {}", result.error);
        return rt;
    }
    return rt;
}

wis::ShaderResource vortex::Texture2D::CreateShaderResource(const vortex::Graphics& gfx) const noexcept
{
    wis::Result result = wis::success;
    if (!_texture) {
        vortex::error("Texture::CreateShaderResource: Texture is not initialized.");
        return {}; // Return an empty shader resource if texture is not initialized
    }
    // Create a shader resource view for the texture
    wis::ShaderResourceDesc srd{
        .format = _format, // Assuming the texture format is already set
        .view_type = wis::TextureViewType::Texture2D,
        .component_mapping = wis::ComponentMapping{
                .r = wis::ComponentSwizzle::Red,
                .g = wis::ComponentSwizzle::Green,
                .b = wis::ComponentSwizzle::Blue,
                .a = wis::ComponentSwizzle::Alpha },
        .subresource_range = wis::SubresourceRange{ .base_mip_level = 0, .level_count = 1, .base_array_layer = 0, .layer_count = 1 }
    };
    wis::ShaderResource shader_resource = gfx.GetDevice().CreateShaderResource(result, _texture, srd);
    if (!success(result)) {
        vortex::error("Texture::CreateShaderResource: Failed to create shader resource view: {}", result.error);
        return {}; // Return an empty shader resource if creation failed
    }
    return shader_resource;
}
