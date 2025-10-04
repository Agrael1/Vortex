#include <fstream>
#include <string>
#include <format>
#include <unordered_map>
#include <tinyxml2.h>
#include <iostream>
#include <filesystem>
#include <span>
#include <bitset>

constexpr inline std::string_view clang_format_exe = CLANG_FORMAT_EXECUTABLE;
void FormatFile(std::filesystem::path file)
{
    constexpr uint32_t repeats = 5;
    if (clang_format_exe.empty()) {
        return;
    }
    std::string cmd = file.string();
    std::cout << "Wisdom Vk Utils: Formatting:\n" << file << '\n';
    std::string command = std::format("\"{}\" -i --style=file {}", clang_format_exe, cmd);

    int ret = 0;
    for (uint32_t i = 0; (ret = std::system(command.c_str())) != 0 && i < repeats; ++i)
        ;
    if (ret != 0) {
        std::cout << "Wisdom Vk Utils: failed to format files with error <" << ret << ">\n";
    }
}

static auto to_pascal_case(std::string_view snake_case) -> std::string
{
    std::string result;
    bool capitalize_next = true;

    for (char c : snake_case) {
        if (c == '_') {
            capitalize_next = true;
        } else if (capitalize_next) {
            result += std::toupper(c);
            capitalize_next = false;
        } else {
            result += c;
        }
    }
    return result;
};

class Generator
{
public:
    Generator() = default;
    void GenerateProperties(std::filesystem::path xml_file_path, std::filesystem::path output_file)
    {
        auto xml_file = xml_file_path.string();

        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(xml_file.c_str()) != tinyxml2::XML_SUCCESS) {
            throw std::runtime_error("Failed to load XML file: " + xml_file);
        }
        tinyxml2::XMLElement* root = doc.RootElement();
        if (!root) {
            throw std::runtime_error("Invalid XML structure: No root element found.");
        }
        std::ofstream ofs(output_file);
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open output file: " + output_file.string());
        }
        ofs << std::format("// Generated properties from {}\n#pragma once\n\n",
                           xml_file_path.filename().string());
        ofs << "#include <cstdint>\n"
            << "#include <string>\n"
            << "#include <filesystem>\n"
            << "#include <optional>\n"
            << "#include <any>\n"
            << "#include <vortex/util/reflection.h>\n"
            << "#include <vortex/util/log.h>\n"
            << "#include <DirectXMath.h>\n"
            << "#include <frozen/unordered_map.h>\n"
            << "#include <frozen/string.h>\n\n";

        ofs << "namespace vortex {\n";

        // Iterate through each property in the XML
        for (tinyxml2::XMLElement* prop = root->FirstChildElement("enum"); prop;
             prop = prop->NextSiblingElement("enum")) {
            const char* name = prop->Attribute("name");
            if (!name) {
                throw std::runtime_error("Property missing 'name' attribute.");
            }
            ofs << GenerateEnum(prop, name);
        }

        for (tinyxml2::XMLElement* prop = root->FirstChildElement("node"); prop;
             prop = prop->NextSiblingElement("node")) {
            const char* name = prop->Attribute("name");
            if (!name) {
                throw std::runtime_error("Node missing 'name' attribute.");
            }
            ofs << GenerateClass(prop, name);
        }

        ofs << "}\n"; // Close namespace vortex
    }
    std::string GenerateClass(tinyxml2::XMLElement* prop, std::string_view name)
    {
        // Generate class definition
        std::string bclass = std::format("struct {}Properties {{UpdateNotifier notifier; // "
                                         "Callback for property change notifications\npublic:\n",
                                         name);
        std::string aclass;
        std::string optional_attributes;
        std::string setters;
        std::string getters;
        std::string notify_property_change; // switch case
        std::string set_property_stub; // stub for set_property
        std::string set_property_stub_any; // stub for set_property with std::any

        std::string frozen_hash = std::format(
                "static constexpr auto property_map = frozen::make_unordered_map<frozen::string, "
                "int>({{\n",
                name);

        uint32_t prop_count = 0;

        std::vector<std::string_view> property_names;

        // Iterate through each property in the node
        for (tinyxml2::XMLElement* child = prop->FirstChildElement("property"); child;
             child = child->NextSiblingElement("property")) {
            const char* prop_name = child->Attribute("name");
            const char* prop_type = child->Attribute("type");
            const char* default_value = child->Attribute("default");
            const char* ui_description = child->Attribute("ui_desc");
            const char* ui_name = child->Attribute("ui_name");
            const char* disabled = child->Attribute("disabled");
            if (!prop_name || !prop_type) {
                throw std::runtime_error("Property missing 'name' or 'type' attribute.");
            }

            property_names.push_back(prop_name);
            frozen_hash += std::format("    {{\"{}\", {}}},\n", prop_name, prop_count);

            // Check if the type is a standard type
            std::string_view prop_type_trsf;
            auto it = standard_types.find(prop_type);
            prop_type_trsf = it != standard_types.end() ? it->second : prop_type;

            default_value = default_value ? default_value : "";
            ui_description = ui_description ? ui_description : "";
            ui_name = ui_name ? ui_name : prop_name;

            // UI description and name to documentation comments
            std::string ui_description_str;
            if (ui_description && *ui_description) {
                ui_description_str = std::format("UI attribute - {}: {}", ui_name, ui_description);
            } else {
                ui_description_str = std::format("UI attribute - {}", ui_name);
            }

            std::string_view transform_type = prop_type_trsf;
            auto transform_it = transform_types.find(prop_type);
            if (transform_it != transform_types.end()) {
                transform_type = transform_it->second;
            }

            // transform name to PascalCase
            std::string pascal_prop_name = to_pascal_case(prop_name);

            // Generate property declaration
            bool is_optional = disabled;
            bool is_disabled = is_optional && std::string_view(disabled) != "false";
            if (is_optional) {
                aclass += std::format("    std::optional<{}> {} {{ {}{} }};\t//<{} \n",
                                      prop_type_trsf,
                                      prop_name,
                                      is_disabled ? "" : prop_type_trsf,
                                      is_disabled ? "" : "{" + std::string(default_value) + "}",
                                      ui_description_str);

                setters += std::format(
                        "void Set{}(std::optional<{}> value, bool notify = false) {{ {} = value; "
                        "if (notify){{ NotifyPropertyChange({}); }} }}\n",
                        pascal_prop_name,
                        transform_type,
                        prop_name,
                        prop_count);
                getters += std::format("template<typename Self>\nstd::optional<{}> Get{}(this "
                                       "Self&& self) {{ return self.{}; }}\n",
                                       transform_type,
                                       pascal_prop_name,
                                       prop_name);

            } else {
                aclass += std::format("    {} {} {{ {} }};\t//<{} \n",
                                      prop_type_trsf,
                                      prop_name,
                                      default_value,
                                      ui_description_str);
                setters += std::format("void Set{}({} value, bool notify = false) {{ {} = {}; if "
                                       "(notify){{ NotifyPropertyChange({}); }}}}\n",
                                       pascal_prop_name,
                                       transform_type,
                                       prop_name,
                                       prop_type_trsf != transform_type
                                               ? std::format("{}{{value}}", prop_type_trsf)
                                               : "value",
                                       prop_count);
                getters += std::format("template<typename Self>\n{} Get{}(this Self&& self) {{ "
                                       "return self.{}; }}\n",
                                       transform_type,
                                       pascal_prop_name,
                                       prop_name);
            }
            notify_property_change += std::format(
                    "case {}: self.notifier({}, "
                    "vortex::reflection_traits<{}>::serialize(self.Get{}()));break;\n",
                    prop_count,
                    prop_count,
                    transform_type,
                    pascal_prop_name);
            set_property_stub += std::format(
                    "case {}: if ({} out_value; "
                    "vortex::reflection_traits<{}>::deserialize(&out_value, value)) {{"
                    "self.Set{}(out_value, notify); }}break;",
                    prop_count,
                    transform_type,
                    transform_type,
                    pascal_prop_name);
            set_property_stub_any += std::format(
                    "case {}: if (auto out_value = std::any_cast<{}>(value)) {{"
                    "self.Set{}(*out_value, notify); }}break;",
                    prop_count,
                    transform_type,
                    pascal_prop_name);
            prop_count++;
        }
        frozen_hash += "});\n";
        if (!optional_attributes.empty()) {
            aclass += "\npublic:";
            aclass += optional_attributes;
        }
        if (!setters.empty()) {
            aclass += "\npublic:\n";
            aclass += setters;
        }
        if (!getters.empty()) {
            aclass += "\npublic:\n";
            aclass += getters;
        }

        // Generate NotifyPropertyChange method
        aclass += "\npublic:\n";
        aclass += std::format("template<typename Self>void NotifyPropertyChange(this Self&& "
                              "self,uint32_t index) {{\n"
                              "    if (!self.notifier) {{\n"
                              "        vortex::error(\"{}: Notifier callback is not set.\");\n"
                              "        return; // No notifier set, cannot notify\n"
                              "    }}\n"
                              "    switch (index) {{\n"
                              "{}"
                              "    default:\n"
                              "        vortex::error(\"{}: Invalid property index for "
                              "notification: {}\", index);\n"
                              "        break;\n"
                              "    }}\n"
                              "}}\n",
                              name,
                              notify_property_change,
                              name,
                              "{}");
        // Generate SetPropertyStub method
        aclass += "\npublic:\n";
        aclass += std::format(
                "template<typename Self>void SetPropertyStub(this Self&& self,uint32_t index, "
                "std::string_view value, bool notify = false) {{\n"
                "    switch (index) {{\n"
                "{}"
                "    default:\n"
                "        vortex::error(\"{}: Invalid property index: {}\", index);\n"
                "        break; // Invalid index, cannot set property\n"
                "    }}\n"
                "}}\n",
                set_property_stub,
                name,
                "{}");
        // Generate SetPropertyStub method with std::any
        aclass += "\npublic:\n";
        aclass += std::format(
                "template<typename Self>void SetPropertyStub(this Self&& self,uint32_t index, "
                "std::any value, bool notify = false) {{\n"
                "    switch (index) {{\n"
                "{}"
                "    default:\n"
                "        vortex::error(\"{}: Invalid property index: {}\", index);\n"
                "        break; // Invalid index, cannot set property\n"
                "    }}\n"
                "}}\n",
                set_property_stub_any,
                name,
                "{}");
        // Generate Serialize method
        aclass += GenerateSerialization(property_names);

        aclass += "};\n";
        return bclass + frozen_hash + aclass;
    }
    std::string GenerateEnum(tinyxml2::XMLElement* prop, std::string_view name)
    {
        // Generate enum definition
        std::string aenum = std::format("enum class {} {{\n", name);
        std::string enum_meta = std::format("template<>struct enum_traits<{}>{{static constexpr "
                                            "std::string_view strings[] = {{",
                                            name);

        uint32_t enum_count = 0;
        // Iterate through each enum value in the node
        for (tinyxml2::XMLElement* child = prop->FirstChildElement("value"); child;
             child = child->NextSiblingElement("value")) {
            const char* value_name = child->Attribute("name");
            const char* value = child->Attribute("value");
            const char* value_description = child->Attribute("ui_desc");
            const char* value_ui_name = child->Attribute("ui_name");

            std::string_view value_ui_name_sv = value_ui_name ? value_ui_name : value_name;
            // Check if the value has a name
            if (!value_name) {
                throw std::runtime_error("Enum value missing 'name' or 'value' attribute.");
            }

            aenum += std::format("    {},\t//<UI name - {}: \n",
                                 value_name,
                                 value_ui_name_sv,
                                 value_description ? value_description : "");

            enum_meta += std::format("\"{}\", ", value_ui_name_sv);
        }
        aenum += "};\n";
        enum_meta += "};\n};\n";

        return aenum + enum_meta;
    }
    std::string GenerateSerialization(std::span<std::string_view> properties)
    {
        // Json serialization code generation
        std::string args;
        std::string serialize_code = "template<typename Self>\n";
        serialize_code += "std::string Serialize(this Self& self) {\n";
        serialize_code += "    return std::format(\"{{ ";
        for (size_t i = 0; i < properties.size(); ++i) {
            const auto& prop = properties[i];
            serialize_code += std::format("{}: {{}}", prop);
            args += std::format(
                    "vortex::reflection_traits<decltype(self.Get{}())>::serialize(self.Get{}())",
                    to_pascal_case(prop),
                    to_pascal_case(prop));
            if (i < properties.size() - 1) {
                serialize_code += ", ";
                args += ",\n";
            }
        }
        serialize_code += "}}\",\n" + args + ");\n";
        serialize_code += "}\n";

        // A bit brutal deserialization code generation
        std::string deserialize_code = "template<typename Self>\n";
        deserialize_code +=
                "bool Deserialize(this Self& self, SerializedProperties values, bool notify) {\n";
        deserialize_code += "    for (auto &&[k,v] : values) {\n";
        deserialize_code += "        size_t index = self.property_map.at(k);\n";
        deserialize_code += "        self.SetPropertyStub(index, v, notify);\n";
        deserialize_code += "}\nreturn true;}\n";
        return serialize_code + deserialize_code;
    }

    std::string GenerateTypeEnum()
    {
        std::string type_enum;
        // Generate all types with enum traits (simulation of reflection)
        type_enum = "enum class PropertyType {\n";
        for (const auto& [key, val] : standard_types) {
            // Make first letter uppercase
            type_enum += std::format("    {},\n", to_pascal_case(key));
        }
        type_enum += "};\n";
        return type_enum;
    }
    std::string GenerateTypeMap()
    {
        std::string type_map;
        return type_map;
    }
    void GenerateTypeTraits(std::filesystem::path output_file)
    {
        std::ofstream ofs(output_file);
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open output file: " + output_file.string());
        }
        ofs << std::format("// Generated type traits\n#pragma once\n\n");
        ofs << "#include <cstdint>\n"
            << "#include <string>\n"
            << "#include <string_view>\n"
            << "#include <filesystem>\n"
            << "#include <optional>\n"
            << "#include <vortex/util/reflection.h>\n"
            << "#include <vortex/util/log.h>\n"
            << "#include <DirectXMath.h>\n"
            << "#include <any>\n\n";
        ofs << "namespace vortex {\n";
        ofs << GenerateTypeEnum();
        ofs << "template<PropertyType T> struct type_traits;\n\n";
        for (const auto& [key, val] : standard_types) {
            ofs << std::format(
                    "template<> struct type_traits<PropertyType::{}> {{ using type = {}; }};\n",
                    to_pascal_case(key),
                    val);
        }

        // Bridge function
        ofs << "\ntemplate<template<PropertyType> typename Exec, typename ...Args>\n";
        ofs << "decltype(auto) bridge(PropertyType type, Args&&...args) {\n";
        ofs << "    switch(type) {\n";
        for (const auto& [key, val] : standard_types) {
            ofs << std::format("        case PropertyType::{}: return Exec<PropertyType::{}>{{}}(std::forward<Args>(args)...); break;\n",
                               to_pascal_case(key),
                               to_pascal_case(key));
        }
        ofs << "        default: vortex::warn(\"Unsupported CefValueType: {}\", reflect::enum_name(type));\n";
        ofs << "    }\n";
        ofs << "}\n";
        ofs << "\n} // namespace vortex\n";
    }

private:
    static inline const std::unordered_map<std::string_view, std::string_view> standard_types{
        {       "bool",                "bool" },
        {       "void",                "void" },
        {         "u8",             "uint8_t" },
        {        "u16",            "uint16_t" },
        {        "u32",            "uint32_t" },
        {        "u64",            "uint64_t" },
        {         "i8",              "int8_t" },
        {        "i16",             "int16_t" },
        {        "i32",             "int32_t" },
        {        "i64",             "int64_t" },

        {        "f32",               "float" },
        {        "f64",              "double" },

        // vector types
        {      "f32x2",   "DirectX::XMFLOAT2" },
        {      "f32x3",   "DirectX::XMFLOAT3" },
        {      "f32x4",   "DirectX::XMFLOAT4" },

        // int vector types
        {      "i32x2",     "DirectX::XMINT2" },
        {      "i32x3",     "DirectX::XMINT3" },
        {      "i32x4",     "DirectX::XMINT4" },

        {      "u32x2",    "DirectX::XMUINT2" },
        {      "u32x3",    "DirectX::XMUINT3" },
        {      "u32x4",    "DirectX::XMUINT4" },

        {   "u8string",         "std::string" },
        {  "u16string",        "std::wstring" },
        {       "path",         "std::string" },

        {      "color",   "DirectX::XMFLOAT4" }, // TODO: XMCOLOR?
        { "quaternion",   "DirectX::XMFLOAT4" },
        {       "rect",   "DirectX::XMFLOAT4" },
        {       "size",   "DirectX::XMFLOAT2" },
        {      "sizei",     "DirectX::XMINT2" },
        {      "sizeu",    "DirectX::XMUINT2" },
        {    "point2d",   "DirectX::XMFLOAT2" },
        {    "point3d",   "DirectX::XMFLOAT3" },
        {      "point",   "DirectX::XMFLOAT2" },
        {     "matrix", "DirectX::XMFLOAT4X4" },

        // Add enum types later
    };
    static inline const std::unordered_map<std::string_view, std::string_view> transform_types{
        {  "u8string",  "std::string_view" },
        { "u16string", "std::wstring_view" },
        {      "path",  "std::string_view" },
    };
};

int main(int argc, char** argv)
{
    // if (argc < 2) {
    //     std::cerr << "Usage: generator <input_xml_file> <output_header_file> or generator "
    //                  "<output_header_file>\n";
    //     return 1;
    // }
    if (argc == 1) {
        // Just generate type traits
        try {
            Generator generator;
            generator.GenerateTypeTraits(DEFAULT_PROPERTY_DIR "type_traits.h");
            FormatFile(DEFAULT_PROPERTY_DIR "type_traits.h");
            std::cout << "Type traits generated successfully.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 1;
        }
        return 0;
    }

    try {
        Generator generator;
        generator.GenerateProperties(argv[1], argv[2]);
        FormatFile(argv[2]);
        std::cout << "Properties generated successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}