// Generated properties from props.xml
#pragma once

#include <cstdint>
#include <string>
#include <filesystem>
#include <optional>
#include <vortex/util/reflection.h>
#include <vortex/util/log.h>
#include <DirectXMath.h>

namespace vortex {
struct ImageInputProperties {
    notifier_callback notifier; // Callback for property change notifications
public:
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
            self.notifier(0, vortex::reflection_traits<decltype(self.image_path)>::serialize(self.image_path));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<decltype(self.image_size)>::serialize(self.image_size));
            break;
        case 2:
            self.notifier(2, vortex::reflection_traits<decltype(self.origin)>::serialize(self.origin));
            break;
        case 3:
            self.notifier(3, vortex::reflection_traits<decltype(self.rotation_2d)>::serialize(self.rotation_2d));
            break;
        case 4:
            self.notifier(4, vortex::reflection_traits<decltype(self.crop_rect)>::serialize(self.crop_rect));
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
};
} // namespace vortex
namespace vortex {
struct WindowOutputProperties {
    notifier_callback notifier; // Callback for property change notifications
public:
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
            self.notifier(0, vortex::reflection_traits<decltype(self.name)>::serialize(self.name));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<decltype(self.window_size)>::serialize(self.window_size));
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
};
} // namespace vortex
namespace vortex {
struct NDIOutputProperties {
    notifier_callback notifier; // Callback for property change notifications
public:
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
            self.notifier(0, vortex::reflection_traits<decltype(self.name)>::serialize(self.name));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<decltype(self.window_size)>::serialize(self.window_size));
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
};
} // namespace vortex
