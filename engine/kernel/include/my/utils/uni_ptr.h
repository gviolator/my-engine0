// #my_engine_source_file
#pragma once
#include <memory>
#include <tuple>
#include <utility>

#include "my/diag/check.h"
#include "my/rtti/ptr.h"
#include "my/utils/scope_guard.h"


namespace my
{
    template <template <typename, typename...> class>
    struct FromUniPtr;

    template <typename Pointer>
    concept ToUniPtrConvertible = requires(Pointer&& ptr) {
        toUniPtr(std::forward<Pointer>(ptr));
    };

    // template <template <typename, typename...> class PointerWrapper>

    // struct FromUniPtrConversion
    // {
    //   constexpr static bool IsDefined = false;
    // };

    template <typename T>
    class UniPtr
    {
    public:
        using ReleaseFunc = void (*)(T*) noexcept;

        UniPtr() = default;

        UniPtr(std::nullptr_t) :
            UniPtr{}
        {
        }

        UniPtr(T* value, ReleaseFunc releaseFunc, uintptr_t customData) :
            m_value(value),
            m_releaseFunc(releaseFunc),
            m_customData(customData)
        {
            MY_DEBUG_CHECK(m_value);
            MY_DEBUG_CHECK(m_releaseFunc);
        }

        UniPtr(UniPtr<T>&& other) :
            m_value(std::exchange(other.m_value, nullptr)),
            m_releaseFunc(std::exchange(other.m_releaseFunc, nullptr)),
            m_customData(std::exchange(other.m_customData, 0))
        {
        }

        UniPtr(const UniPtr<T>&) = delete;

        template <typename Pointer>
        // requires(std::is_same_v<T, typename Pointer::element_type>)
        UniPtr(Pointer&& smartPtr)  //: UniPtr(toUniPtr(std::forward<Pointer>(ptr)))
        {
            auto [rawPtr, release, d] = release_ptr<T>(std::forward<Pointer>(smartPtr));
            m_value = rawPtr;
            m_releaseFunc = release;
            m_customData = d;
        }

        ~UniPtr()
        {
            reset();
        }

        UniPtr<T>& operator=(UniPtr<T>&& other) noexcept
        {
            MY_DEBUG_CHECK(m_value == nullptr && m_releaseFunc == nullptr, "Re-assignment for non null UniPtr supposed to be invalid operation");
            m_value = std::exchange(other.m_value, nullptr);
            m_releaseFunc = std::exchange(other.m_releaseFunc, nullptr);
            m_customData = std::exchange(other.m_customData, 0);

            return *this;
        }

        UniPtr<T>& operator=(const UniPtr<T>& other) = delete;

        UniPtr<T>& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        template <typename SmartPointer>
        UniPtr<T>& operator=(SmartPointer&& smartPtr)
        {
            reset();
            auto [rawPtr, release, d] = release_ptr<T>(std::forward<SmartPointer>(smartPtr));
            m_value = rawPtr;
            m_releaseFunc = release;
            m_customData = d;

            return *this;
        }

        explicit operator bool() const noexcept
        {
            return m_value != nullptr;
        }

        void reset() noexcept
        {
            if (m_value)
            {
                const ReleaseFunc releaseFunc = std::exchange(m_releaseFunc, nullptr);
                MY_DEBUG_FATAL(releaseFunc);

                m_customData = 0;
                T* const value = std::exchange(m_value, nullptr);
                releaseFunc(value);
            }
        }

        template <typename Pointer>
        Pointer release()
        {
            scope_on_leave
            {
                m_value = nullptr;
                m_releaseFunc = nullptr;
                m_customData = 0;
            };

            Pointer smartPtr;
            if (m_value)
            {
                takes_ownership(smartPtr, m_value, m_customData);
            }
            return smartPtr;
        }

    private:
        std::pair<T*, uintptr_t> release() &&
        {
            m_releaseFunc = nullptr;
            return {
                std::exchange(m_value, nullptr),
                std::exchange(m_customData, 0)};
        }

        T* m_value = nullptr;
        ReleaseFunc m_releaseFunc = nullptr;
        uintptr_t m_customData = 0;
    };

    template <typename T>
    UniPtr(std::unique_ptr<T>&&) -> UniPtr<T>;

}  // namespace my

namespace std
{
    template <typename U, typename T>
    std::tuple<U*, void (*)(U*) noexcept, uintptr_t> release_ptr(std::unique_ptr<T>&& smartPtr)
    {
        constexpr uintptr_t UniquePtrMarker = 101;
        if (!smartPtr)
        {
            return {};
        }

        auto release = [](U* value) noexcept
        {
            delete value;
        };

        T* const ptr = smartPtr.release();
        U* targetPtr = nullptr;

        if constexpr (std::is_same_v<T, U> || std::is_convertible_v<T*, U*>)
        {
            targetPtr = static_cast<U*>(ptr);
        }
        else if constexpr (std::is_base_of_v<my::IRttiObject, T>)
        {
            targetPtr = ptr->template as<U*>();
            MY_DEBUG_CHECK(targetPtr);
        }
        else
        {
            // NoConversionWay should always be false
            constexpr bool NoConversionWay = std::is_convertible_v<T*, U*> || std::is_base_of_v<my::IRttiObject, T>;
            static_assert(NoConversionWay);
        }

        return {targetPtr, release, UniquePtrMarker};
    }

    template <typename U, typename T>
    void takes_ownership(std::unique_ptr<T>& smartPtr, U* ptr, [[maybe_unused]] uintptr_t marker)
    {
        [[maybe_unused]] constexpr uintptr_t UniquePtrMarker = 101;
        MY_DEBUG_FATAL(marker == UniquePtrMarker);

        if (!ptr)
        {
            smartPtr = std::unique_ptr<T>{};
            return;
        }

        if constexpr (std::is_same_v<T, U> || std::is_convertible_v<U*, T*>)
        {
            smartPtr = std::unique_ptr<T>{static_cast<T*>(ptr)};
        }
        else if constexpr (std::is_base_of_v<my::IRttiObject, U>)
        {
            T* const targetPtr = ptr->template as<T*>();
            MY_DEBUG_CHECK(targetPtr);
            smartPtr = std::unique_ptr<T>{targetPtr};
        }
        else
        {
            // NoConversionWay should always be false
            constexpr bool NoConversionWay = std::is_convertible_v<U*, T*> || std::is_base_of_v<my::IRttiObject, T>;
            static_assert(NoConversionWay);
        }
    }

}  // namespace std