// #my_engine_source_file
#include "my/memory/mem_allocator.h"
#include "my/rtti/rtti_impl.h"

namespace my
{

    class CrtAllocator final : public IAlignedMemAllocator
    {
    public:
        MY_REFCOUNTED_CLASS(my::CrtAllocator, IAlignedMemAllocator)

        ~ CrtAllocator() = default;

        size_t getAllocationAlignment() const override
        {
            return alignof(std::max_align_t);
        }

        void* alloc(size_t size) override
        {
            void* const newPtr = ::malloc(size);
            return newPtr;
        }

        void* realloc(void* oldPtr, size_t size) override
        {
            void* const newPtr = ::realloc(oldPtr, size);
            return newPtr;
        }

        void* allocAligned(size_t size, size_t alignment) override
        {
            MY_DEBUG_ASSERT(is_power_of2(alignment));

            return ::_aligned_malloc(size, alignment);
        }

        void* reallocAligned(void* oldPtr, size_t size, size_t alignment) override
        {
            MY_DEBUG_ASSERT(is_power_of2(alignment));

#ifdef _WIN32
            auto const ptr = ::_aligned_realloc(oldPtr, size, alignment);
            return ptr;
#elif defined(_LIBCPP_HAS_ALIGNED_ALLOC)

            void* newPtr = nullptr;

            if (!alignment)
            {
                newPtr = ::realloc(ptr, size);
            }
            else
            {
                G_ASSERT((!ptr, "Can not realoc with alignment");
                newPtr = aligned_alloc(*alignment, size);
            }

            G_ASSERT(!alignment || reinterpret_cast<ptrdiff_t>(newPtr) % *alignment == 0);
            return newPtr;
#else
            auto const newPtr = ::malloc(size);
            G_ASSERT(reinterpret_cast<ptrdiff_t>(newPtr) % *alignment == 0);
            return newPtr;

#endif
        }

        /**
         */
        void free(void* ptr) override
        {
            ::free(ptr);
        }

        void freeAligned(void* ptr, [[maybe_unused]] size_t) override
        {
            ::_aligned_free(ptr);
        }
    };

//-----------------------------------------------------------------------------
#if 0
AllocatorMemoryResource::AllocatorMemoryResource(AllocatorProvider allocProvider): m_allocatorProvider(allocProvider)
{
	G_ASSERT(m_allocatorProvider);
}

void* AllocatorMemoryResource::do_allocate(size_t size_, size_t align_)
{
	return allocator().alloc(size_, align_);
}

void AllocatorMemoryResource::do_deallocate(void* ptr_, size_t size_, size_t align_)
{
	allocator().free(ptr_, size_);
}

bool AllocatorMemoryResource::do_is_equal(const std::pmr::memory_resource& other_) const noexcept
{
	if (static_cast<const std::pmr::memory_resource*>(this) == &other_)
	{
		return true;
	}

	if (const AllocatorMemoryResource* const wrapper = dynamic_cast<const AllocatorMemoryResource*>(&other_); wrapper)
	{
		return &wrapper->allocator() == &this->allocator();
	}
		
	return false;
}

Allocator& AllocatorMemoryResource::allocator() const
{
	Allocator* const alloc_ = m_allocatorProvider();
	G_ASSERT(alloc_);

	return *alloc_;
}
#endif

    //my::MemAllocatorPtr g_defaultAllocatorInstance;

    IAlignedMemAllocator& getSystemAllocator()
    {
        static Ptr<CrtAllocator> crtAllocator = rtti::createInstanceSingleton<CrtAllocator>();
        return *crtAllocator;
    }

    //const MemAllocatorPtr& getDefaultAllocator()
    //{
    //    return g_defaultAllocatorInstance ? g_defaultAllocatorInstance : getCrtAllocator();
    //}

    // void SetDefaultAllocator(Allocator::Ptr allocator)
    // {
    //     defaultAllocatorInstance = std::move(allocator);
    // }
}  // namespace my
