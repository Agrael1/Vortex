#include <vortex/gfx/texture.h>
#include <vortex/graphics.h>
#include <vortex/util/log.h>

wis::RenderTarget vortex::Texture2D::CreateRenderTarget(vortex::Graphics& gfx) const noexcept
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
