// Generated properties from props.xml
#pragma once

#include <cstdint>
#include <string>
#include <filesystem>
#include <optional>
#include <vortex/util/reflection.h>
#include <vortex/util/log.h>
#include <DirectXMath.h>
#include <frozen/unordered_map.h>
#include <frozen/string.h>

namespace vortex {
enum class BlendChannel {
    All, //<UI name - All Channels:
    Red, //<UI name - Red:
    Green, //<UI name - Green:
    Blue, //<UI name - Blue:
    Alpha, //<UI name - Alpha:
    RGB, //<UI name - RGB Only:
};
template<>
struct enum_traits<BlendChannel> {
    static constexpr std::string_view strings[] = {
        "All Channels",
        "Red",
        "Green",
        "Blue",
        "Alpha",
        "RGB Only",
    };
};
enum class BlendMode {
    Normal, //<UI name - Normal:
    Multiply, //<UI name - Multiply:
    Screen, //<UI name - Screen:
    Overlay, //<UI name - Overlay:
    Add, //<UI name - Add:
    Subtract, //<UI name - Subtract:
    Difference, //<UI name - Difference:
    Darken, //<UI name - Darken:
    Lighten, //<UI name - Lighten:
};
template<>
struct enum_traits<BlendMode> {
    static constexpr std::string_view strings[] = {
        "Normal",
        "Multiply",
        "Screen",
        "Overlay",
        "Add",
        "Subtract",
        "Difference",
        "Darken",
        "Lighten",
    };
};
struct BlendProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto property_map = frozen::make_unordered_map<frozen::string, int>({
            { "blend_mode", 0 },
            { "blend_channel", 1 },
            { "opacity", 2 },
            { "clamp_result", 3 },
    });
    BlendMode blend_mode{ BlendMode::Normal }; //<UI attribute - Blend Mode: How to combine the images.
    BlendChannel blend_channel{ BlendChannel::All }; //<UI attribute - Blend Channel: Which channels to use from source.
    float opacity{ 1.0 }; //<UI attribute - Opacity: Overall opacity of the blend.
    bool clamp_result{ true }; //<UI attribute - Clamp Result: Clamp output to [0,1] range.

public:
    void SetBlendMode(BlendMode value, bool notify = false)
    {
        blend_mode = value;
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetBlendChannel(BlendChannel value, bool notify = false)
    {
        blend_channel = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }
    void SetOpacity(float value, bool notify = false)
    {
        opacity = value;
        if (notify) {
            NotifyPropertyChange(2);
        }
    }
    void SetClampResult(bool value, bool notify = false)
    {
        clamp_result = value;
        if (notify) {
            NotifyPropertyChange(3);
        }
    }

public:
    template<typename Self>
    BlendMode GetBlendMode(this Self&& self)
    {
        return self.blend_mode;
    }
    template<typename Self>
    BlendChannel GetBlendChannel(this Self&& self)
    {
        return self.blend_channel;
    }
    template<typename Self>
    float GetOpacity(this Self&& self)
    {
        return self.opacity;
    }
    template<typename Self>
    bool GetClampResult(this Self&& self)
    {
        return self.clamp_result;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("Blend: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(0, vortex::reflection_traits<BlendMode>::serialize(self.GetBlendMode()));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<BlendChannel>::serialize(self.GetBlendChannel()));
            break;
        case 2:
            self.notifier(2, vortex::reflection_traits<float>::serialize(self.GetOpacity()));
            break;
        case 3:
            self.notifier(3, vortex::reflection_traits<bool>::serialize(self.GetClampResult()));
            break;
        default:
            vortex::error("Blend: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (BlendMode out_value; vortex::reflection_traits<BlendMode>::deserialize(&out_value, value)) {
                self.SetBlendMode(out_value, notify);
                break;
            }
        case 1:
            if (BlendChannel out_value; vortex::reflection_traits<BlendChannel>::deserialize(&out_value, value)) {
                self.SetBlendChannel(out_value, notify);
                break;
            }
        case 2:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetOpacity(out_value, notify);
                break;
            }
        case 3:
            if (bool out_value; vortex::reflection_traits<bool>::deserialize(&out_value, value)) {
                self.SetClampResult(out_value, notify);
                break;
            }
        default:
            vortex::error("Blend: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return "{" + "blend_mode:" + vortex::reflection_traits<Self>::serialize(self.GetBlendMode()) + "," + "blend_channel:" + vortex::reflection_traits<Self>::serialize(self.GetBlendChannel()) + "," + "opacity:" + vortex::reflection_traits<Self>::serialize(self.GetOpacity()) + "," + "clamp_result:" + vortex::reflection_traits<Self>::serialize(self.GetClampResult()) + "}";
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            size_t index = self.property_map.at(k);
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct ImageInputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto property_map = frozen::make_unordered_map<frozen::string, int>({
            { "image_path", 0 },
            { "image_size", 1 },
            { "origin", 2 },
            { "rotation_2d", 3 },
            { "crop_rect", 4 },
    });
    std::string image_path{}; //<UI attribute - Image Path: Path to image file.
    DirectX::XMFLOAT2 image_size{}; //<UI attribute - Image Size: Size of the image in pixels.
    DirectX::XMFLOAT2 origin{}; //<UI attribute - Origin: Origin (Anchor) point of the image.
    float rotation_2d{ 0.0 }; //<UI attribute - Rotation (2D): Rotation of the image in degrees.
    std::optional<DirectX::XMFLOAT4> crop_rect{}; //<UI attribute - Crop Rectangle: Rectangle to crop the image.

public:
    void SetImagePath(std::string_view value, bool notify = false)
    {
        image_path = std::string{ value };
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetImageSize(DirectX::XMFLOAT2 value, bool notify = false)
    {
        image_size = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }
    void SetOrigin(DirectX::XMFLOAT2 value, bool notify = false)
    {
        origin = value;
        if (notify) {
            NotifyPropertyChange(2);
        }
    }
    void SetRotation2d(float value, bool notify = false)
    {
        rotation_2d = value;
        if (notify) {
            NotifyPropertyChange(3);
        }
    }
    void SetCropRect(std::optional<DirectX::XMFLOAT4> value, bool notify = false)
    {
        crop_rect = value;
        if (notify) {
            NotifyPropertyChange(4);
        }
    }

public:
    template<typename Self>
    std::string_view GetImagePath(this Self&& self)
    {
        return self.image_path;
    }
    template<typename Self>
    DirectX::XMFLOAT2 GetImageSize(this Self&& self)
    {
        return self.image_size;
    }
    template<typename Self>
    DirectX::XMFLOAT2 GetOrigin(this Self&& self)
    {
        return self.origin;
    }
    template<typename Self>
    float GetRotation2d(this Self&& self)
    {
        return self.rotation_2d;
    }
    template<typename Self>
    std::optional<DirectX::XMFLOAT4> GetCropRect(this Self&& self)
    {
        return self.crop_rect;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("ImageInput: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(0, vortex::reflection_traits<std::string_view>::serialize(self.GetImagePath()));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetImageSize()));
            break;
        case 2:
            self.notifier(2, vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetOrigin()));
            break;
        case 3:
            self.notifier(3, vortex::reflection_traits<float>::serialize(self.GetRotation2d()));
            break;
        case 4:
            //self.notifier(4, vortex::reflection_traits<DirectX::XMFLOAT4>::serialize(self.GetCropRect()));
            break;
        default:
            vortex::error("ImageInput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value; vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetImagePath(out_value, notify);
                break;
            }
        case 1:
            if (DirectX::XMFLOAT2 out_value; vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetImageSize(out_value, notify);
                break;
            }
        case 2:
            if (DirectX::XMFLOAT2 out_value; vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetOrigin(out_value, notify);
                break;
            }
        case 3:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetRotation2d(out_value, notify);
                break;
            }
        case 4:
            if (DirectX::XMFLOAT4 out_value; vortex::reflection_traits<DirectX::XMFLOAT4>::deserialize(&out_value, value)) {
                self.SetCropRect(out_value, notify);
                break;
            }
        default:
            vortex::error("ImageInput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return "{" + "image_path:" + vortex::reflection_traits<Self>::serialize(self.GetImagePath()) + "," + "image_size:" + vortex::reflection_traits<Self>::serialize(self.GetImageSize()) + "," + "origin:" + vortex::reflection_traits<Self>::serialize(self.GetOrigin()) + "," + "rotation_2d:" + vortex::reflection_traits<Self>::serialize(self.GetRotation2d()) + "," + "crop_rect:" + vortex::reflection_traits<Self>::serialize(self.GetCropRect()) + "}";
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            size_t index = self.property_map.at(k);
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct WindowOutputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto property_map = frozen::make_unordered_map<frozen::string, int>({
            { "name", 0 },
            { "window_size", 1 },
    });
    std::string name{ "Vortex Output Window" }; //<UI attribute - Window Title: Title of the output window.
    DirectX::XMUINT2 window_size{ 1920, 1080 }; //<UI attribute - Window Size: Resolution of the output window.

public:
    void SetName(std::string_view value, bool notify = false)
    {
        name = std::string{ value };
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetWindowSize(DirectX::XMUINT2 value, bool notify = false)
    {
        window_size = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }

public:
    template<typename Self>
    std::string_view GetName(this Self&& self)
    {
        return self.name;
    }
    template<typename Self>
    DirectX::XMUINT2 GetWindowSize(this Self&& self)
    {
        return self.window_size;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("WindowOutput: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(0, vortex::reflection_traits<std::string_view>::serialize(self.GetName()));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<DirectX::XMUINT2>::serialize(self.GetWindowSize()));
            break;
        default:
            vortex::error("WindowOutput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value; vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetName(out_value, notify);
                break;
            }
        case 1:
            if (DirectX::XMUINT2 out_value; vortex::reflection_traits<DirectX::XMUINT2>::deserialize(&out_value, value)) {
                self.SetWindowSize(out_value, notify);
                break;
            }
        default:
            vortex::error("WindowOutput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return "{" + "name:" + vortex::reflection_traits<Self>::serialize(self.GetName()) + "," + "window_size:" + vortex::reflection_traits<Self>::serialize(self.GetWindowSize()) + "}";
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            size_t index = self.property_map.at(k);
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct NDIOutputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto property_map = frozen::make_unordered_map<frozen::string, int>({
            { "name", 0 },
            { "window_size", 1 },
    });
    std::string name{ "Vortex NDI Output" }; //<UI attribute - NDI Name: Name of the NDI stream.
    DirectX::XMUINT2 window_size{ 1920, 1080 }; //<UI attribute - Window Size: Resolution of the output window.

public:
    void SetName(std::string_view value, bool notify = false)
    {
        name = std::string{ value };
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetWindowSize(DirectX::XMUINT2 value, bool notify = false)
    {
        window_size = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }

public:
    template<typename Self>
    std::string_view GetName(this Self&& self)
    {
        return self.name;
    }
    template<typename Self>
    DirectX::XMUINT2 GetWindowSize(this Self&& self)
    {
        return self.window_size;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("NDIOutput: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(0, vortex::reflection_traits<std::string_view>::serialize(self.GetName()));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<DirectX::XMUINT2>::serialize(self.GetWindowSize()));
            break;
        default:
            vortex::error("NDIOutput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value; vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetName(out_value, notify);
                break;
            }
        case 1:
            if (DirectX::XMUINT2 out_value; vortex::reflection_traits<DirectX::XMUINT2>::deserialize(&out_value, value)) {
                self.SetWindowSize(out_value, notify);
                break;
            }
        default:
            vortex::error("NDIOutput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return "{" + "name:" + vortex::reflection_traits<Self>::serialize(self.GetName()) + "," + "window_size:" + vortex::reflection_traits<Self>::serialize(self.GetWindowSize()) + "}";
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            size_t index = self.property_map.at(k);
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
} // namespace vortex
