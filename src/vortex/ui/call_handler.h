#pragma once
#include <vortex/ui/implements.h>
#include <vortex/ui/value.h>
#include <include/cef_v8.h>
#include <include/cef_values.h>
#include <vortex/util/log.h>
#include <include/cef_parser.h>

namespace vortex::ui {
class VortexV8Handler : public CefImplements<VortexV8Handler, CefV8Handler>
{
public:
    VortexV8Handler()
        : context(CefV8Context::GetCurrentContext())
    {
        auto global = context->GetGlobal();
        global->SetValue("vortexCall",
                         CefV8Value::CreateFunction("vortexCall", this),
                         V8_PROPERTY_ATTRIBUTE_NONE);

        global->SetValue("vortexCallAsync",
                         CefV8Value::CreateFunction("vortexCallAsync", this),
                         V8_PROPERTY_ATTRIBUTE_NONE);
    }

public:
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override
    {
        if (name == "vortexCall") {
            // Assume there is at least one argument with name of called function
            if (arguments.size() < 1 || !arguments[0]->IsString()) {
                exception = "Invalid arguments";
                return false; // Invalid arguments
            }

            auto a = CefProcessMessage::Create(arguments[0]->GetStringValue());
            // The second argument shoul be a JSON string with the arguments
            if (arguments.size() < 2) {
                CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, a);
                return true; // No arguments, just call the function
            }

            auto args = a->GetArgumentList();
            args->SetSize(arguments.size() - 1); // Set size to number of arguments excluding the function name
            for (size_t i = 1; i < arguments.size(); ++i) {
                if (arguments[i]->IsString()) {
                    args->SetString(i - 1, arguments[i]->GetStringValue());
                } else if (arguments[i]->IsInt()) {
                    args->SetInt(i - 1, arguments[i]->GetIntValue());
                } else if (arguments[i]->IsDouble()) {
                    args->SetDouble(i - 1, arguments[i]->GetDoubleValue());
                } else if (arguments[i]->IsBool()) {
                    args->SetBool(i - 1, arguments[i]->GetBoolValue());
                } else if (arguments[i]->IsObject()) {
                    // Handle objects by serializing them to JSON
                    CefRefPtr<CefValue> value = CefValue::Create();
                    value->SetInt(0); // Placeholder for object serialization
                    CefString json_string = CefWriteJSON(value, JSON_WRITER_DEFAULT);
                    args->SetString(i - 1, json_string);
                } else if (arguments[i]->IsNull()) {
                    args->SetNull(i - 1);
                } else {
                    exception = "Unsupported argument type";
                    vortex::error("Unsupported argument type in vortexCall");
                    return false; // Unsupported argument type
                }
            }

            CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, a);
            retval = CefV8Value::CreateBool(true); // Indicate success
            return true; // Script executed successfully
        }
        if (name == "vortexCallAsync") {
            // Similar to vortexCall, but handle async calls
            if (arguments.size() < 1 || !arguments[0]->IsString()) {
                exception = "Invalid arguments";
                return false; // Invalid arguments
            }
            auto a = CefProcessMessage::Create(arguments[0]->GetStringValue());
            auto args = a->GetArgumentList();
            args->SetSize(arguments.size() - 1); // Set size to number of arguments excluding the function name + 1 for promise
            for (size_t i = 1; i < arguments.size(); ++i) {
                if (arguments[i]->IsString()) {
                    args->SetString(i - 1, arguments[i]->GetStringValue());
                } else if (arguments[i]->IsInt()) {
                    args->SetInt(i - 1, arguments[i]->GetIntValue());
                } else if (arguments[i]->IsDouble()) {
                    args->SetDouble(i - 1, arguments[i]->GetDoubleValue());
                } else if (arguments[i]->IsBool()) {
                    args->SetBool(i - 1, arguments[i]->GetBoolValue());
                } else if (arguments[i]->IsObject()) {
                    // Handle objects by serializing them to JSON
                    CefRefPtr<CefValue> value = CefValue::Create();
                    value->SetInt(0); // Placeholder for object serialization
                    CefString json_string = CefWriteJSON(value, JSON_WRITER_DEFAULT);
                    args->SetString(i - 1, json_string);
                } else if (arguments[i]->IsNull()) {
                    args->SetNull(i - 1);
                } else {
                    exception = "Unsupported argument type";
                    vortex::error("Unsupported argument type in vortexCall");
                    return false; // Unsupported argument type
                }
            }

            promise = CefV8Value::CreatePromise();

            CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, a);

            retval = promise; // Indicate success
            return true; // Script executed successfully
        }
        return false; // Function not found
    }

    void ResolvePromise(CefRefPtr<CefListValue> value)
    {
        if (promise->IsPromise()) {
            context->Enter();
            promise->ResolvePromise(bridge<v8_value_traits>(std::move(value), 0));
            context->Exit();
        } else {
            vortex::error("Attempted to resolve a non-promise value");
        }
    }

private:
    CefRefPtr<CefV8Value> promise;
    CefRefPtr<CefV8Context> context;
};
} // namespace vortex::ui
