#pragma once
#include <include/cef_base.h>

namespace vortex::ui {
template<typename CRTP, typename... Args>
class CefImplements : public Args...
{
public:
    void AddRef() const override
    {
        ref_count_.AddRef();
    }
    bool Release() const override
    {
        if (ref_count_.Release()) {
            delete static_cast<const CRTP*>(this);
            return true;
        }
        return false;
    }
    bool HasOneRef() const override
    {
        return ref_count_.HasOneRef();
    }
    bool HasAtLeastOneRef() const override
    {
        return ref_count_.HasAtLeastOneRef();
    }

private:
    CefRefCount ref_count_;
};
} // namespace vortex::ui