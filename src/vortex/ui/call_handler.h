#pragma once
#include <vortex/ui/implements.h>
#include <vortex/ui/value.h>
#include <include/cef_v8.h>
#include <include/cef_values.h>
#include <vortex/util/log.h>
#include <include/cef_parser.h>
#include <vortex/ui/message_routing.h>
#include <utility>

namespace vortex::ui {
class PromiseRegistry {
public:
    virtual ~PromiseRegistry() = default;
    virtual uint64_t RegisterPromise(CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Value> resolver) = 0;
};

class VortexV8Handler : public CefImplements<VortexV8Handler, CefV8Handler>
{
public:
    explicit VortexV8Handler(PromiseRegistry& owner)
        : _owner(owner)
    {
        auto context = CefV8Context::GetCurrentContext();
        if (!context)
            return;

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
        auto current_context = CefV8Context::GetCurrentContext();
        if (!current_context || !current_context->IsValid()) {
            exception = "V8 context is not available";
            vortex::error("vortexCall request '{}' failed: current V8 context is invalid", name.ToString());
            return false;
        }

        if (name == "vortexCall") {
            // Assume there is at least one argument with name of called function
            if (arguments.size() < 1 || !arguments[0]->IsString()) {
                exception = "Invalid arguments";
                return false; // Invalid arguments
            }

            auto a = CefProcessMessage::Create(arguments[0]->GetStringValue());
            // The second argument shoul be a JSON string with the arguments
            if (arguments.size() < 2) {
                current_context->GetFrame()->SendProcessMessage(PID_BROWSER, a);
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

            current_context->GetFrame()->SendProcessMessage(PID_BROWSER, a);
            retval = CefV8Value::CreateBool(true); // Indicate success
            return true; // Script executed successfully
        }
        if (name == "vortexCallAsync") {
            // Similar to vortexCall, but handle async calls
            if (arguments.size() < 1 || !arguments[0]->IsString()) {
                exception = "Invalid arguments";
                return false; // Invalid arguments
            }
            auto promise = CefV8Value::CreatePromise();
            if (!promise) {
                exception = "Failed to create promise";
                vortex::error("vortexCallAsync '{}' failed: unable to allocate V8 promise", arguments[0]->GetStringValue().ToString());
                return false;
            }

            const uint64_t request_id = RegisterPromise(current_context, promise);
            if (request_id == 0) {
                exception = "Failed to register promise";
                return false;
            }
            auto a = CefProcessMessage::Create(BuildRoutedMessageName(arguments[0]->GetStringValue(), request_id));
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

            DispatchProcessMessage(current_context, std::move(a));
            retval = promise; // Indicate success
            return true; // Script executed successfully
        }
        return false; // Function not found
    }

private:
    uint64_t RegisterPromise(CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Value> promise)
    {
        return _owner.RegisterPromise(std::move(context), std::move(promise));
    }

    void DispatchProcessMessage(CefRefPtr<CefV8Context> call_context, CefRefPtr<CefProcessMessage> message)
    {
        if (!message) {
            vortex::error("Attempted to dispatch an empty CEF message");
            return;
        }
        if (!call_context || !call_context->IsValid()) {
            vortex::error("V8 context is invalid while dispatching message");
            return;
        }

        auto frame = call_context->GetFrame();
        if (!frame) {
            vortex::error("Failed to dispatch message: no frame available");
            return;
        }
        frame->SendProcessMessage(PID_BROWSER, std::move(message));
    }

    PromiseRegistry& _owner;
};
} // namespace vortex::ui
