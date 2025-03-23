// #my_engine_source_header
#pragma once
#include <memory>
#include <utility>

#include "my/diag/check.h"
#include "my/rtti/ptr.h"

namespace my
{
  template <template <typename, typename...> class>
  struct FromUniPtr;

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

    template <template <typename, typename...> class>
    friend struct FromUniPtr;
  };

  namespace kernel_detail
  {
    template <template <typename, typename...> class Pointer>
    struct UniPtrCast;

    template <>
    struct UniPtrCast<std::unique_ptr>
    {
      static constexpr uintptr_t UniquePtrMarker = 1001;

      template <typename T>
      static std::unique_ptr<T> get(T* value, [[maybe_unused]] const uintptr_t custom)
      {
        MY_DEBUG_FATAL(custom == UniquePtrMarker);
        return std::unique_ptr<T>{value};
      }
    };
  }  // namespace kernel_detail

  template <template <typename, typename...> class Pointer>
  struct FromUniPtr
  {
    template <typename T>
    static Pointer<T> get(UniPtr<T>&& ptr)
    {
      if (!ptr)
      {
        return nullptr;
      }

      auto [value, customData] = std::move(ptr).release();
      return kernel_detail::UniPtrCast<Pointer>::get(value, customData);
    }
  };

  template <typename T>
  std::unique_ptr<T> toUniquePtr(my::UniPtr<T>&& ptr)
  {
    return FromUniPtr<std::unique_ptr>::get(std::move(ptr));
  }

}  // namespace my

namespace std
{
  template <typename T>
  my::UniPtr<T> toUniPtr(std::unique_ptr<T>&& ptr)
  {
    if (!ptr)
    {
      return {};
    }

    auto release = [](T* value) noexcept
    {
      delete value;
    };

    return my::UniPtr<T>{ptr.release(), release, my::kernel_detail::UniPtrCast<std::unique_ptr>::UniquePtrMarker};
  }
}  // namespace std