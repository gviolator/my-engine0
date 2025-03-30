// #my_engine_source_file

#pragma once
#include "my/diag/check.h"

namespace my::kernel_detail
{
    /**
     * @brief Singleton memory operations helper.
     *
     * This template struct provides customized new and delete operators to enforce singleton allocation
     * for the specified class type. It ensures that only one instance of the class can be allocated.
     *
     * @tparam Class The class type for which singleton memory operations are to be provided.
     */
    template <typename Class>
    struct SingletonMemOp
    {
        /**
         * @brief Allocates memory for a singleton instance.
         *
         * This method overrides the global new operator to allocate memory for a singleton instance.
         * It ensures that the size of the allocated memory is not greater than the size of the storage.
         *
         * @param size The size of the memory to be allocated.
         * @return A pointer to the allocated memory.
         */
        static void* operator_new([[maybe_unused]] size_t size) noexcept
        {
            decltype(auto) state = getSingletonState();
            MY_FATAL(size <= sizeof(decltype(state.storage)));
            MY_FATAL(!state.allocated, "Singleton already allocated");
            state.allocated = true;
            return &state.storage;
        }

        /**
         * @brief Deallocates memory for a singleton instance.
         *
         * This method overrides the global delete operator to deallocate memory for a singleton instance.
         * It ensures that the memory being deallocated was previously allocated by this class.
         *
         * @param ptr The pointer to the memory to be deallocated.
         * @param size The size of the memory to be deallocated (ignored).
         */
        static void operator_delete([[maybe_unused]] void* ptr, size_t) noexcept
        {
            decltype(auto) state = getSingletonState();
            MY_DEBUG_CHECK(state.allocated);
            MY_DEBUG_CHECK(ptr == &state.storage);
            state.allocated = false;

            memset(ptr, 0, sizeof(Class));
        }

    private:
        static auto& getSingletonState()
        {
            alignas(Class) static struct
            {
                alignas(Class) std::byte storage[sizeof(Class)];
                bool allocated = false;
            } state = {};
            return (state);
        }
    };

}  // namespace my::kernel_detail

/**
 * @brief Macro to declare singleton memory operations for a class.
 *
 * This macro defines custom new and delete operators for the specified class to enforce singleton allocation.
 * Use the MY_DECLARE_SINGLETON_MEMOP macro within your class definition to enable singleton memory operations.
 * class MySingletonClass
 * {
 *      MY_DECLARE_SINGLETON_MEMOP(MySingletonClass)
 * private:
 *      MySingletonClass() = default;
 *      ~MySingletonClass() = default;
 * public:
 *      static MySingletonClass& getInstance()
 *      {
 *          static MySingletonClass instance;
 *          return instance;
 *      }
 * };
 * @param ClassName The name of the class for which singleton memory operations are to be declared.
 */
#define MY_DECLARE_SINGLETON_MEMOP(ClassName)                                                    \
public:                                                                                          \
    static void* operator new(size_t size)                                                       \
    {                                                                                            \
        static_assert(!std::is_abstract_v<ClassName>, "Using singleton memop on abstract type"); \
        return ::my::kernel_detail::SingletonMemOp<ClassName>::operator_new(size);               \
    }                                                                                            \
                                                                                                 \
    static void operator delete(void* ptr, size_t size)                                          \
    {                                                                                            \
        ::my::kernel_detail::SingletonMemOp<ClassName>::operator_delete(ptr, size);              \
    }
