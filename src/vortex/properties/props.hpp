// Generated properties from props.xml
#pragma once

#include <vortex/properties/type_traits.h>
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
enum class LUTInterp {
    Nearest, //<UI name - Nearest:
    Trilinear, //<UI name - Trilinear:
    Tetrahedral, //<UI name - Tetrahedral:
};
template<>
struct enum_traits<LUTInterp> {
    static constexpr std::string_view strings[] = {
        "Nearest",
        "Trilinear",
        "Tetrahedral",
    };
};
struct BlendProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    {      "blend_mode",   { 0, PropertyType::I32 } },
                    { "blend_constants", { 1, PropertyType::Color } },
                    {    "clamp_result",  { 2, PropertyType::Bool } },
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
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
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

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetBlendMode(static_cast<BlendMode>(std::get<int32_t>(value)), notify);
            break;
        case 1:
            self.SetBlendConstants(std::get<DirectX::XMFLOAT4>(value), notify);
            break;
        case 2:
            self.SetClampResult(std::get<bool>(value), notify);
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
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct SelectProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    { "input_index", { 0, PropertyType::I32 } },
    });
    int32_t input_index{ 0 }; //<UI attribute - Input Index: Index of the input to select.

public:
    void SetInputIndex(int32_t value, bool notify = false)
    {
        input_index = value;
        if (notify) {
            NotifyPropertyChange(0);
        }
    }

public:
    template<typename Self>
    int32_t GetInputIndex(this Self&& self)
    {
        return self.input_index;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("Select: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(0, vortex::reflection_traits<int32_t>::serialize(self.GetInputIndex()));
            break;
        default:
            vortex::error("Select: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            if (int32_t out_value;
                vortex::reflection_traits<int32_t>::deserialize(&out_value, value)) {
                self.SetInputIndex(out_value, notify);
            }
            break;
        default:
            vortex::error("Select: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetInputIndex(std::get<int32_t>(value), notify);
            break;
        default:
            vortex::error("Select: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format("{{ input_index: {}}}",
                           vortex::reflection_traits<decltype(self.GetInputIndex())>::serialize(
                                   self.GetInputIndex()));
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct TransformProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    { "translation", { 0, PropertyType::F32x2 } },
                    {       "scale", { 1, PropertyType::F32x2 } },
                    {       "pivot", { 2, PropertyType::F32x2 } },
                    {    "rotation",   { 3, PropertyType::F32 } },
                    {   "crop_rect",  { 4, PropertyType::Rect } },
    });
    DirectX::XMFLOAT2 translation{ 0.0, 0.0 }; //<UI attribute - Translation: Translation offset in
                                               //pixels.
    DirectX::XMFLOAT2 scale{ 1.0, 1.0 }; //<UI attribute - Scale: Scaling factors.
    DirectX::XMFLOAT2 pivot{ 0.5, 0.5 }; //<UI attribute - Pivot Point: Pivot point for rotation and
                                         //scaling (normalized coordinates 0-1).
    float rotation{ 0.0 }; //<UI attribute - Rotation: Rotation angle in degrees.
    DirectX::XMFLOAT4 crop_rect{ 0.0, 0.0, 1.0, 1.0 }; //<UI attribute - Crop Rectangle: Crop
                                                       //rectangle (normalized coordinates 0-1).

public:
    void SetTranslation(DirectX::XMFLOAT2 value, bool notify = false)
    {
        translation = value;
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetScale(DirectX::XMFLOAT2 value, bool notify = false)
    {
        scale = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }
    void SetPivot(DirectX::XMFLOAT2 value, bool notify = false)
    {
        pivot = value;
        if (notify) {
            NotifyPropertyChange(2);
        }
    }
    void SetRotation(float value, bool notify = false)
    {
        rotation = value;
        if (notify) {
            NotifyPropertyChange(3);
        }
    }
    void SetCropRect(DirectX::XMFLOAT4 value, bool notify = false)
    {
        crop_rect = value;
        if (notify) {
            NotifyPropertyChange(4);
        }
    }

public:
    template<typename Self>
    DirectX::XMFLOAT2 GetTranslation(this Self&& self)
    {
        return self.translation;
    }
    template<typename Self>
    DirectX::XMFLOAT2 GetScale(this Self&& self)
    {
        return self.scale;
    }
    template<typename Self>
    DirectX::XMFLOAT2 GetPivot(this Self&& self)
    {
        return self.pivot;
    }
    template<typename Self>
    float GetRotation(this Self&& self)
    {
        return self.rotation;
    }
    template<typename Self>
    DirectX::XMFLOAT4 GetCropRect(this Self&& self)
    {
        return self.crop_rect;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("Transform: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(
                    0,
                    vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetTranslation()));
            break;
        case 1:
            self.notifier(1,
                          vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetScale()));
            break;
        case 2:
            self.notifier(2,
                          vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.GetPivot()));
            break;
        case 3:
            self.notifier(3, vortex::reflection_traits<float>::serialize(self.GetRotation()));
            break;
        case 4:
            self.notifier(
                    4,
                    vortex::reflection_traits<DirectX::XMFLOAT4>::serialize(self.GetCropRect()));
            break;
        default:
            vortex::error("Transform: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            if (DirectX::XMFLOAT2 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetTranslation(out_value, notify);
            }
            break;
        case 1:
            if (DirectX::XMFLOAT2 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetScale(out_value, notify);
            }
            break;
        case 2:
            if (DirectX::XMFLOAT2 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&out_value, value)) {
                self.SetPivot(out_value, notify);
            }
            break;
        case 3:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetRotation(out_value, notify);
            }
            break;
        case 4:
            if (DirectX::XMFLOAT4 out_value;
                vortex::reflection_traits<DirectX::XMFLOAT4>::deserialize(&out_value, value)) {
                self.SetCropRect(out_value, notify);
            }
            break;
        default:
            vortex::error("Transform: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetTranslation(std::get<DirectX::XMFLOAT2>(value), notify);
            break;
        case 1:
            self.SetScale(std::get<DirectX::XMFLOAT2>(value), notify);
            break;
        case 2:
            self.SetPivot(std::get<DirectX::XMFLOAT2>(value), notify);
            break;
        case 3:
            self.SetRotation(std::get<float>(value), notify);
            break;
        case 4:
            self.SetCropRect(std::get<DirectX::XMFLOAT4>(value), notify);
            break;
        default:
            vortex::error("Transform: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format(
                "{{ translation: {}, scale: {}, pivot: {}, rotation: {}, crop_rect: {}}}",
                vortex::reflection_traits<decltype(self.GetTranslation())>::serialize(
                        self.GetTranslation()),
                vortex::reflection_traits<decltype(self.GetScale())>::serialize(self.GetScale()),
                vortex::reflection_traits<decltype(self.GetPivot())>::serialize(self.GetPivot()),
                vortex::reflection_traits<decltype(self.GetRotation())>::serialize(
                        self.GetRotation()),
                vortex::reflection_traits<decltype(self.GetCropRect())>::serialize(
                        self.GetCropRect()));
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct ColorCorrectionProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    { "brightness",  { 0, PropertyType::I32 } },
                    {   "contrast",  { 1, PropertyType::I32 } },
                    { "saturation",  { 2, PropertyType::I32 } },
                    {        "lut", { 3, PropertyType::Path } },
                    { "lut_interp",  { 4, PropertyType::I32 } },
    });
    float brightness{ 0.0 }; //<UI attribute - brightness
    float contrast{ 1.0 }; //<UI attribute - contrast
    float saturation{ 1.0 }; //<UI attribute - saturation
    std::string lut{}; //<UI attribute - LUT Path: Path to 3D LUT file.
    LUTInterp lut_interp{ LUTInterp::Trilinear }; //<UI attribute - LUT Interpolation: Algorithm for
                                                  //LUT interpolation.

public:
    void SetBrightness(float value, bool notify = false)
    {
        brightness = value;
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetContrast(float value, bool notify = false)
    {
        contrast = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }
    void SetSaturation(float value, bool notify = false)
    {
        saturation = value;
        if (notify) {
            NotifyPropertyChange(2);
        }
    }
    void SetLut(std::string_view value, bool notify = false)
    {
        lut = std::string{ value };
        if (notify) {
            NotifyPropertyChange(3);
        }
    }
    void SetLutInterp(LUTInterp value, bool notify = false)
    {
        lut_interp = value;
        if (notify) {
            NotifyPropertyChange(4);
        }
    }

public:
    template<typename Self>
    float GetBrightness(this Self&& self)
    {
        return self.brightness;
    }
    template<typename Self>
    float GetContrast(this Self&& self)
    {
        return self.contrast;
    }
    template<typename Self>
    float GetSaturation(this Self&& self)
    {
        return self.saturation;
    }
    template<typename Self>
    std::string_view GetLut(this Self&& self)
    {
        return self.lut;
    }
    template<typename Self>
    LUTInterp GetLutInterp(this Self&& self)
    {
        return self.lut_interp;
    }

public:
    template<typename Self>
    void NotifyPropertyChange(this Self&& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("ColorCorrection: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }
        switch (index) {
        case 0:
            self.notifier(0, vortex::reflection_traits<float>::serialize(self.GetBrightness()));
            break;
        case 1:
            self.notifier(1, vortex::reflection_traits<float>::serialize(self.GetContrast()));
            break;
        case 2:
            self.notifier(2, vortex::reflection_traits<float>::serialize(self.GetSaturation()));
            break;
        case 3:
            self.notifier(3, vortex::reflection_traits<std::string_view>::serialize(self.GetLut()));
            break;
        case 4:
            self.notifier(4, vortex::reflection_traits<LUTInterp>::serialize(self.GetLutInterp()));
            break;
        default:
            vortex::error("ColorCorrection: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetBrightness(out_value, notify);
            }
            break;
        case 1:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetContrast(out_value, notify);
            }
            break;
        case 2:
            if (float out_value; vortex::reflection_traits<float>::deserialize(&out_value, value)) {
                self.SetSaturation(out_value, notify);
            }
            break;
        case 3:
            if (std::string_view out_value;
                vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetLut(out_value, notify);
            }
            break;
        case 4:
            if (LUTInterp out_value;
                vortex::reflection_traits<LUTInterp>::deserialize(&out_value, value)) {
                self.SetLutInterp(out_value, notify);
            }
            break;
        default:
            vortex::error("ColorCorrection: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetBrightness(static_cast<float>(std::get<int32_t>(value)), notify);
            break;
        case 1:
            self.SetContrast(static_cast<float>(std::get<int32_t>(value)), notify);
            break;
        case 2:
            self.SetSaturation(static_cast<float>(std::get<int32_t>(value)), notify);
            break;
        case 3:
            self.SetLut(std::get<std::string>(value), notify);
            break;
        case 4:
            self.SetLutInterp(static_cast<LUTInterp>(std::get<int32_t>(value)), notify);
            break;
        default:
            vortex::error("ColorCorrection: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format(
                "{{ brightness: {}, contrast: {}, saturation: {}, lut: {}, lut_interp: {}}}",
                vortex::reflection_traits<decltype(self.GetBrightness())>::serialize(
                        self.GetBrightness()),
                vortex::reflection_traits<decltype(self.GetContrast())>::serialize(
                        self.GetContrast()),
                vortex::reflection_traits<decltype(self.GetSaturation())>::serialize(
                        self.GetSaturation()),
                vortex::reflection_traits<decltype(self.GetLut())>::serialize(self.GetLut()),
                vortex::reflection_traits<decltype(self.GetLutInterp())>::serialize(
                        self.GetLutInterp()));
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct ImageInputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    { "image_path", { 0, PropertyType::Path } },
    });
    std::string image_path{}; //<UI attribute - Image Path: Path to image file.

public:
    void SetImagePath(std::string_view value, bool notify = false)
    {
        image_path = std::string{ value };
        if (notify) {
            NotifyPropertyChange(0);
        }
    }

public:
    template<typename Self>
    std::string_view GetImagePath(this Self&& self)
    {
        return self.image_path;
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
        default:
            vortex::error("ImageInput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value;
                vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetImagePath(out_value, notify);
            }
            break;
        default:
            vortex::error("ImageInput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetImagePath(std::get<std::string>(value), notify);
            break;
        default:
            vortex::error("ImageInput: Invalid property index: {}", index);
            break; // Invalid index, cannot set property
        }
    }
    template<typename Self>
    std::string Serialize(this Self& self)
    {
        return std::format("{{ image_path: {}}}",
                           vortex::reflection_traits<decltype(self.GetImagePath())>::serialize(
                                   self.GetImagePath()));
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct StreamInputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    {       "stream_url", { 0, PropertyType::U8string } },
                    { "stream_buffering",      { 1, PropertyType::I32 } },
    });
    std::string stream_url{}; //<UI attribute - Stream URL: URL of the video stream.
    int32_t stream_buffering{ 1000 }; //<UI attribute - Buffering: Buffering time in milliseconds.

public:
    void SetStreamUrl(std::string_view value, bool notify = false)
    {
        stream_url = std::string{ value };
        if (notify) {
            NotifyPropertyChange(0);
        }
    }
    void SetStreamBuffering(int32_t value, bool notify = false)
    {
        stream_buffering = value;
        if (notify) {
            NotifyPropertyChange(1);
        }
    }

public:
    template<typename Self>
    std::string_view GetStreamUrl(this Self&& self)
    {
        return self.stream_url;
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
            self.notifier(1,
                          vortex::reflection_traits<int32_t>::serialize(self.GetStreamBuffering()));
            break;
        default:
            vortex::error("StreamInput: Invalid property index for notification: {}", index);
            break;
        }
    }

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view out_value;
                vortex::reflection_traits<std::string_view>::deserialize(&out_value, value)) {
                self.SetStreamUrl(out_value, notify);
            }
            break;
        case 1:
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

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetStreamUrl(std::get<std::string>(value), notify);
            break;
        case 1:
            self.SetStreamBuffering(std::get<int32_t>(value), notify);
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
                "{{ stream_url: {}, stream_buffering: {}}}",
                vortex::reflection_traits<decltype(self.GetStreamUrl())>::serialize(
                        self.GetStreamUrl()),
                vortex::reflection_traits<decltype(self.GetStreamBuffering())>::serialize(
                        self.GetStreamBuffering()));
    }
    template<typename Self>
    bool Deserialize(this Self& self, SerializedProperties values, bool notify)
    {
        for (auto&& [k, v] : values) {
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct WindowOutputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    {        "name", { 0, PropertyType::U8string } },
                    { "window_size",    { 1, PropertyType::Sizeu } },
                    {   "framerate",      { 2, PropertyType::I32 } },
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
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
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

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetName(std::get<std::string>(value), notify);
            break;
        case 1:
            self.SetWindowSize(std::get<DirectX::XMUINT2>(value), notify);
            break;
        case 2:
            self.SetFramerate(static_cast<vortex::ratio32_t>(std::get<int32_t>(value)), notify);
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
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
struct NDIOutputProperties {
    UpdateNotifier notifier; // Callback for property change notifications
public:
    static constexpr auto
            property_map = frozen::make_unordered_map<frozen::string,
                                                      std::pair<uint32_t, PropertyType>>({
                    {        "name", { 0, PropertyType::U8string } },
                    { "window_size",    { 1, PropertyType::Sizeu } },
                    {   "framerate",      { 2, PropertyType::I32 } },
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
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         std::string_view value,
                         bool notify = false)
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

public:
    template<typename Self>
    void SetPropertyStub(this Self&& self,
                         uint32_t index,
                         const PropertyValue& value,
                         bool notify = false)
    {
        switch (index) {
        case 0:
            self.SetName(std::get<std::string>(value), notify);
            break;
        case 1:
            self.SetWindowSize(std::get<DirectX::XMUINT2>(value), notify);
            break;
        case 2:
            self.SetFramerate(static_cast<vortex::ratio32_t>(std::get<int32_t>(value)), notify);
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
            uint32_t index = self.property_map.at(k).first;
            self.SetPropertyStub(index, v, notify);
        }
        return true;
    }
};
} // namespace vortex
