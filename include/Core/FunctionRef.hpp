#pragma once

#include <utility>
#include <functional>
#include <concepts>
#include <type_traits>

namespace nbl {

template <typename Signature>
class FunctionRef;

template <typename R, typename... Args>
class FunctionRef<R(Args...)> {
private:

public:

    FunctionRef() = delete;

    template<typename F> 
    requires (std::is_invocable_r_v<R, F&, Args...> 
           && !std::same_as<std::remove_cvref_t<F>, FunctionRef>)
    FunctionRef(F&& f) noexcept {
        // F is a function pointer
        if constexpr(std::is_pointer_v<std::remove_reference_t<F>>) {
	
            _pObj = reinterpret_cast<void*>(f);
            _pfnFunc = [](void* ptr, Args&&... args) -> R {
                auto fn = reinterpret_cast<std::remove_reference_t<F>>(ptr);
                return std::invoke_r<R>(fn, std::forward<Args>(args)...);
            };
        
        // F is a lambda or another callable with state
        } else {
            _pObj = const_cast<void*>(reinterpret_cast<const void*>(std::addressof(f)));
            _pfnFunc = [](void* ptr, Args&&... args) -> R {
                auto& fn = *reinterpret_cast<std::remove_reference_t<F>*>(ptr);
                return std::invoke_r<R>(fn, std::forward<Args>(args)...);
            };
        }
    }

    FunctionRef(const FunctionRef&) noexcept = default;

    FunctionRef& operator=(const FunctionRef& other) noexcept = default;


    R operator()(Args... args) const {
        return _pfnFunc(_pObj, std::forward<Args>(args)...);
    }

private:

    void* _pObj;
    R(*_pfnFunc)(void*, Args&&...);

};

}
