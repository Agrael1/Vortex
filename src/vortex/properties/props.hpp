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
enum class BlendMode {
    Normal, //<UI name - Normal:
    Multiply, //<UI name - Multiply:
    Screen, //<UI name - Screen:
    Add, //<UI name - Add:
    Subtract, //<UI name - Subtract:
    Darken, //<UI name - Darken:
    Lighten, //<UI name - Lighten:
    Difference, //<UI name - Difference:
    Overlay, //<UI name - Overlay:
};
template<>
struct enum_traits<BlendMode> {
    static constexpr std::string_view strings[] = {
        "Normal", "Multiply", "Screen",     "Add",     "Subtract",
        "Darken", "Lighten",  "Difference", "Overlay",
    };
};
struct BlendProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto property_map = frozen::make_unordered_map<frozen::string, int>({
            {      "blend_mode", 0 },
            { "blend_constants", 1 },
            {    "clamp_result", 2 },
    });
    BlendMode blend_mode{ BlendMode::Normal }; //<UI attribute - Blend Mode: How to combine the
                                               //images.
    DirectX::XMFLOAT4 blend_constants{ 1.0, 1.0, 1.0, 1.0 }; //<UI attribute - Blend Constants:
                                                             //Constants used in certain blend
                                                             //modes.
    bool clamp_result{ true }; //<UI attribute - Clamp Result: Clamp output to [0,1] range.

public:
    void SetBlendMode(BlendMode value, bool notify = false)
    {
        blend_mode = value;
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetBlendConstants(DirectX::XMFLOAT4 value, bool notify = false)
    {
        blend_constants = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }
    void SetClampResult(bool value, bool notify = false)
    {
        clamp_result = value;
        if (notify) {
            NotifyPropertyChange(2);
        }
    }

public:
    template<typename Self>
    BlendMode GetBlendMode(this Self&& self)
    {
        return self.blend_mode;
    }
    template<typename Self>
    DirectX::XMFLOAT4 GetBlendConstants(this Self&& self)
    {
        return self.blend_constants;
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
            self.notifier(1,
                          vortex::reflection_traits<DirectX::XMFLOAT4>::serialize(
                                  self.GetBlendConstants()));
            break;
        case 2:
            self.notifier(2, vortex::reflection_traits<bool>::serialize(self.GetClampResult()));
            break;
        default:
            vortex::error("Blend: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void
    SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (BlendMode out_value;
                vortex::reflection_traits<BlendMode>::deserialize(&out_value, value)) {
                self.SetBlendMode(out_value, notify);
            }
            break;
        case 1:
            if (DirectX::XMFLOAT4 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT4>::deserialize(&out_value, value)) {
                self.SetBlendConstants(out_value, notify);
            }
            break;
        case 2:
            if (bool out_value; vortex::reflection_traits<bool>::deserialize(&out_value, value)) {
                self.SetClampResult(out_value, notify);
            }
            break;
        default:
            vortex::error("Blend: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format("{{ blend_mode: {}, blend_constants: {}, clamp_result: {}}}",
                           vortex::reflection_traits<decltype(self.GetBlendMode())>::serialize(
                                   self.GetBlendMode()),
                           vortex::reflection_traits<decltype(self.GetBlendConstants())>::serialize(
                                   self.GetBlendConstants()),
                           vortex::reflection_traits<decltype(self.GetClampResult())>::serialize(
                                   self.GetClampResult()));
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
            {  "image_path", 0 },
            {  "image_size", 1 },
            {      "origin", 2 },
            { "rotation_2d", 3 },
    });
    std::string image_path{}; //<UI attribute - Image Path: Path to image file.
    DirectX::XMFLOAT2 image_size{}; //<UI attribute - Image Size: Size of the image in pixels.
    DirectX::XMFLOAT2 origin{}; //<UI attribute - Origin: Origin (Anchor) point of the image.
    float rotation_2d{ 0.0 }; //<UI attribute - Rotation (2D): Rotation of the image in degrees.

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
            self.notifier(
                    0,
                    vortex::reflection_traits<std::string_view>::serialize(self.GetImagePath()));
            break;
        case 1:
            self.notifier(
                    1,
                    vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetImageSize()));
            break;
        case 2:
            self.notifier(
                    2,
                    vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetOrigin()));
            break;
        case 3:
            self.notifier(3, vortex::reflection_traits<float>::serialize(self.GetRotation2d()));
            break;
        default:
            vortex::error("ImageInput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void
    SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value;
                vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetImagePath(out_value, notify);
            }
            break;
        case 1:
            if (DirectX::XMFLOAT2 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetImageSize(out_value, notify);
            }
            break;
        case 2:
            if (DirectX::XMFLOAT2 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetOrigin(out_value, notify);
            }
            break;
        case 3:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetRotation2d(out_value, notify);
            }
            break;
        default:
            vortex::error("ImageInput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format(
                "{{ image_path: {}, image_size: {}, origin: {}, rotation_2d: {}}}",
                vortex::reflection_traits<decltype(self.GetImagePath())>::serialize(
                        self.GetImagePath()),
                vortex::reflection_traits<decltype(self.GetImageSize())>::serialize(
                        self.GetImageSize()),
                vortex::reflection_traits<decltype(self.GetOrigin())>::serialize(self.GetOrigin()),
                vortex::reflection_traits<decltype(self.GetRotation2d())>::serialize(
                        self.GetRotation2d()));
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
struct StreamInputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto property_map = frozen::make_unordered_map<frozen::string, int>({
            {       "stream_url", 0 },
            {      "stream_size", 1 },
            {           "origin", 2 },
            {      "rotation_2d", 3 },
            { "stream_buffering", 4 },
    });
    std::string stream_url{}; //<UI attribute - Stream URL: URL of the video stream.
    DirectX::XMFLOAT2 stream_size{}; //<UI attribute - Stream Size: Size of the video stream in
                                     //pixels.
    DirectX::XMFLOAT2 origin{}; //<UI attribute - Origin: Origin (Anchor) point of the stream.
    float rotation_2d{ 0.0 }; //<UI attribute - Rotation (2D): Rotation of the stream in degrees.
    int32_t stream_buffering{ 1000 }; //<UI attribute - Buffering: Buffering time in milliseconds.

public:
    void SetStreamUrl(std::string_view value, bool notify = false)
    {
        stream_url = std::string{ value };
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetStreamSize(DirectX::XMFLOAT2 value, bool notify = false)
    {
        stream_size = value;
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
    void SetStreamBuffering(int32_t value, bool notify = false)
    {
        stream_buffering = value;
        if (notify) {
            NotifyPropertyChange(4);
        }
    }

public:
    template<typename Self>
    std::string_view GetStreamUrl(this Self&& self)
    {
        return self.stream_url;
    }
    template<typename Self>
    DirectX::XMFLOAT2 GetStreamSize(this Self&& self)
    {
        return self.stream_size;
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
    int32_t GetStreamBuffering(this Self&& self)
    {
        return self.stream_buffering;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("StreamInput: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(
                    0,
                    vortex::reflection_traits<std::string_view>::serialize(self.GetStreamUrl()));
            break;
        case 1:
            self.notifier(
                    1,
                    vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetStreamSize()));
            break;
        case 2:
            self.notifier(
                    2,
                    vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetOrigin()));
            break;
        case 3:
            self.notifier(3, vortex::reflection_traits<float>::serialize(self.GetRotation2d()));
            break;
        case 4:
            self.notifier(4,
                          vortex::reflection_traits<int32_t>::serialize(self.GetStreamBuffering()));
            break;
        default:
            vortex::error("StreamInput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void
    SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value;
                vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetStreamUrl(out_value, notify);
            }
            break;
        case 1:
            if (DirectX::XMFLOAT2 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetStreamSize(out_value, notify);
            }
            break;
        case 2:
            if (DirectX::XMFLOAT2 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetOrigin(out_value, notify);
            }
            break;
        case 3:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetRotation2d(out_value, notify);
            }
            break;
        case 4:
            if (int32_t out_value;
                vortex::reflection_traits<int32_t>::deserialize(&out_value, value)) {
                self.SetStreamBuffering(out_value, notify);
            }
            break;
        default:
            vortex::error("StreamInput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format(
                "{{ stream_url: {}, stream_size: {}, origin: {}, rotation_2d: {}, "
                "stream_buffering: {}}}",
                vortex::reflection_traits<decltype(self.GetStreamUrl())>::serialize(
                        self.GetStreamUrl()),
                vortex::reflection_traits<decltype(self.GetStreamSize())>::serialize(
                        self.GetStreamSize()),
                vortex::reflection_traits<decltype(self.GetOrigin())>::serialize(self.GetOrigin()),
                vortex::reflection_traits<decltype(self.GetRotation2d())>::serialize(
                        self.GetRotation2d()),
                vortex::reflection_traits<decltype(self.GetStreamBuffering())>::serialize(
                        self.GetStreamBuffering()));
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
            {        "name", 0 },
            { "window_size", 1 },
            {   "framerate", 2 },
    });
    std::string name{ "Vortex Output Window" }; //<UI attribute - Window Title: Title of the output
                                                //window.
    DirectX::XMUINT2 window_size{ 1920, 1080 }; //<UI attribute - Window Size: Resolution of the
                                                //output window.
    vortex::ratio32_t framerate{ 60,
                                 1 }; //<UI attribute - Framerate: Framerate of the output window.

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
    void SetFramerate(vortex::ratio32_t value, bool notify = false)
    {
        framerate = value;
        if (notify) {
            NotifyPropertyChange(2);
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
    template<typename Self>
    vortex::ratio32_t GetFramerate(this Self&& self)
    {
        return self.framerate;
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
            self.notifier(0,
                          vortex::reflection_traits<std::string_view>::serialize(self.GetName()));
            break;
        case 1:
            self.notifier(
                    1,
                    vortex::reflection_traits<DirectX::XMUINT2>::serialize(self.GetWindowSize()));
            break;
        case 2:
            self.notifier(
                    2,
                    vortex::reflection_traits<vortex::ratio32_t>::serialize(self.GetFramerate()));
            break;
        default:
            vortex::error("WindowOutput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void
    SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value;
                vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetName(out_value, notify);
            }
            break;
        case 1:
            if (DirectX::XMUINT2 out_value;
                vortex::reflection_traits<DirectX::XMUINT2>::deserialize(&out_value, value)) {
                self.SetWindowSize(out_value, notify);
            }
            break;
        case 2:
            if (vortex::ratio32_t out_value;
                vortex::reflection_traits<vortex::ratio32_t>::deserialize(&out_value, value)) {
                self.SetFramerate(out_value, notify);
            }
            break;
        default:
            vortex::error("WindowOutput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format(
                "{{ name: {}, window_size: {}, framerate: {}}}",
                vortex::reflection_traits<decltype(self.GetName())>::serialize(self.GetName()),
                vortex::reflection_traits<decltype(self.GetWindowSize())>::serialize(
                        self.GetWindowSize()),
                vortex::reflection_traits<decltype(self.GetFramerate())>::serialize(
                        self.GetFramerate()));
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
            {        "name", 0 },
            { "window_size", 1 },
            {   "framerate", 2 },
    });
    std::string name{ "Vortex NDI Output" }; //<UI attribute - NDI Name: Name of the NDI stream.
    DirectX::XMUINT2 window_size{ 1920, 1080 }; //<UI attribute - Window Size: Resolution of the
                                                //output window.
    vortex::ratio32_t framerate{ 60,
                                 1 }; //<UI attribute - Framerate: Framerate of the output window.

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
    void SetFramerate(vortex::ratio32_t value, bool notify = false)
    {
        framerate = value;
        if (notify) {
            NotifyPropertyChange(2);
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
    template<typename Self>
    vortex::ratio32_t GetFramerate(this Self&& self)
    {
        return self.framerate;
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
            self.notifier(0,
                          vortex::reflection_traits<std::string_view>::serialize(self.GetName()));
            break;
        case 1:
            self.notifier(
                    1,
                    vortex::reflection_traits<DirectX::XMUINT2>::serialize(self.GetWindowSize()));
            break;
        case 2:
            self.notifier(
                    2,
                    vortex::reflection_traits<vortex::ratio32_t>::serialize(self.GetFramerate()));
            break;
        default:
            vortex::error("NDIOutput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void
    SetPropertyStub(this Self&& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value;
                vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetName(out_value, notify);
            }
            break;
        case 1:
            if (DirectX::XMUINT2 out_value;
                vortex::reflection_traits<DirectX::XMUINT2>::deserialize(&out_value, value)) {
                self.SetWindowSize(out_value, notify);
            }
            break;
        case 2:
            if (vortex::ratio32_t out_value;
                vortex::reflection_traits<vortex::ratio32_t>::deserialize(&out_value, value)) {
                self.SetFramerate(out_value, notify);
            }
            break;
        default:
            vortex::error("NDIOutput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format(
                "{{ name: {}, window_size: {}, framerate: {}}}",
                vortex::reflection_traits<decltype(self.GetName())>::serialize(self.GetName()),
                vortex::reflection_traits<decltype(self.GetWindowSize())>::serialize(
                        self.GetWindowSize()),
                vortex::reflection_traits<decltype(self.GetFramerate())>::serialize(
                        self.GetFramerate()));
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
