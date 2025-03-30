// #my_engine_source_file

#include "my/memory/buffer.h"

#include "my/diag/check.h"
#include "my/memory/fixed_size_block_allocator.h"
#include "my/memory/mem_allocator.h"
#include "my/utils/scope_guard.h"

// #define TRACK_BUFFER_ALLOCATIONS

namespace my
{
    /**
     */
    struct BufferBase::Header
    {
        inline static constexpr uint32_t BufferToken = 0x11AA22BBui32;

        std::atomic<uint32_t> refs{1ui32};
        uint32_t capacity;
        uint32_t size;
        uint32_t token = BufferToken;
    };

    namespace
    {
        using BufferHeader = BufferBase::Header;

        constexpr size_t HeaderAlignment = sizeof(ptrdiff_t);
        constexpr size_t AllocationGranularity = 256;
        constexpr size_t BigAllocationGranularity = 1024;
        constexpr size_t BigAllocationThreshold = 4096;

        constexpr size_t HeaderSize = sizeof(std::aligned_storage_t<sizeof(BufferHeader), alignof(BufferHeader)>);
        constexpr ptrdiff_t ClientDataOffset = HeaderSize;

#ifdef TRACK_BUFFER_ALLOCATIONS
        class BufferAllocationTracker
        {
        public:
            ~BufferAllocationTracker()
            {
            }

            void notifyAllocated(size_t size)
            {
            }

            void notifyFree(size_t size)
            {
            }

        private:
            std::mutex m_mutex;
            size_t m_currentAllocationSize = 0;
        };
#endif

        /**
         */
        class BufferAllocatorHolder
        {
        public:
            BufferAllocatorHolder() :
                m_hostMemory(createHostVirtualMemory(my::Megabyte(5), true))
            {
                const auto createAlloc = [&](size_t blockSize)
                {
                    return AllocatorEntry{blockSize, createFixedSizeBlockAllocator(m_hostMemory, blockSize, true)};
                };

                m_blockAllocators[0] = createAlloc(256);
                m_blockAllocators[1] = createAlloc(512);
                m_blockAllocators[2] = createAlloc(1024);
                m_blockAllocators[3] = createAlloc(2048);
            }

            MemAllocator& getAllocator(size_t size)
            {
                for (auto& [blockSize, allocator] : m_blockAllocators)
                {
                    if (size <= blockSize)
                    {
                        return *allocator;
                    }
                }

                return getSystemAllocator();
            }

        private:
            using AllocatorEntry = std::tuple<size_t, MemAllocatorPtr>;

            HostMemoryPtr m_hostMemory;
            std::array<AllocatorEntry, 4> m_blockAllocators;
        };


        uint32_t refsCount(const BufferHeader* header)
        {
            return header == nullptr ? 0 : header->refs.load(std::memory_order_relaxed);
        }

        std::byte* clientData(const BufferHeader* header)
        {
            if (!header || header->size == 0)
            {
                return nullptr;
            }

            MY_FATAL(header->capacity > 0);

            BufferHeader* const mutableHeader = const_cast<BufferHeader*>(header);
            return reinterpret_cast<std::byte*>(mutableHeader) + ClientDataOffset;
        }

        MemAllocator& getBufferAllocator(size_t storageSize)
        {
            static BufferAllocatorHolder g_bufferAllocatorHolder;
            return g_bufferAllocatorHolder.getAllocator(storageSize);
        }

        MemAllocator& getBufferAllocator(const BufferBase::Header& header)
        {
            MY_DEBUG_FATAL(header.capacity > 0);

            const size_t storageSize = HeaderSize + header.capacity;
            return getBufferAllocator(storageSize);
        }


    }  // namespace

    BufferHandle BufferStorage::allocate(size_t clientSize)
    {
        const size_t granuleSize = (clientSize < BigAllocationThreshold) ? AllocationGranularity : BigAllocationGranularity;
        const size_t storageSize = alignedSize(HeaderSize + clientSize, granuleSize);
        const size_t capacity = storageSize - HeaderSize;

        MemAllocator& allocator = getBufferAllocator(storageSize);
        void* const storage = allocator.alloc(storageSize);
        MY_FATAL(storage);
        MY_FATAL(reinterpret_cast<ptrdiff_t>(storage) % HeaderAlignment == 0);

        BufferHeader* header = new(storage) BufferHeader;
        header->capacity = capacity;
        header->size = clientSize;

        return header;
    }

    BufferHandle BufferStorage::reallocate(BufferHandle buffer, size_t newSize)
    {
        if (buffer == nullptr)
        {
            return BufferStorage::allocate(newSize);
        }

        BufferHeader* const header = reinterpret_cast<BufferHeader*>(buffer);
        MY_DEBUG_FATAL(header->refs.load(std::memory_order_relaxed) == 1);
        MY_DEBUG_FATAL(header->capacity > 0);

        if (header->capacity >= newSize)
        {
            header->size = newSize;
            return buffer;
        }

        getBufferAllocator(*header).free(buffer);
        return BufferStorage::allocate(newSize);
    }

    void BufferStorage::release(BufferHandle& buffer)
    {
        if (!buffer)
        {
            return;
        }

        BufferHeader* const header = buffer;
        if (header->refs.fetch_sub(1) == 1)
        {
            getBufferAllocator(*header).free(header);
        }

        buffer = nullptr;
    }

    BufferHandle BufferStorage::takeOut(BufferBase&& buffer)
    {
        if (!buffer.m_storage)
        {
            return nullptr;
        }

        MY_DEBUG_FATAL(refsCount(buffer.m_storage) == 1);

        BufferHeader* const storage = std::exchange(buffer.m_storage, nullptr);
        return storage;
    }

    void* BufferStorage::getClientData(BufferHandle handle)
    {
        return my::clientData(handle);
    }

    size_t BufferStorage::getClientSize(const BufferHandle handle)
    {
        return handle == nullptr ? 0 : handle->size;
    }

    Buffer BufferStorage::bufferFromHandle(BufferHandle handle)
    {
        MY_DEBUG_CHECK(handle);
        return Buffer(handle);
    }

    Buffer BufferStorage::bufferFromClientData(void* ptr, std::optional<size_t> size)
    {
        MY_FATAL(ptr);

        BufferHeader* const header = reinterpret_cast<BufferHeader*>(ptr) - ClientDataOffset;
        Buffer buffer{header};
        if (size)
        {
            MY_DEBUG_FATAL(*size <= header->capacity, "Size ({}) does not fit to the buffer with capacity ({})", *size, header->capacity);
            buffer.resize(*size);
        }

        return buffer;
    }

    BufferBase::BufferBase() :
        m_storage(nullptr)
    {
    }

    BufferBase::BufferBase(BufferHeader* header) :
        m_storage(header)
    {
        MY_FATAL(!m_storage || refsCount(m_storage) > 0);
    }

    BufferBase::~BufferBase()
    {
        release();
    }

    size_t BufferBase::size() const
    {
        return m_storage ? m_storage->size : 0;
    }

    bool BufferBase::empty() const
    {
        return !m_storage || m_storage->size == 0;
    }

    BufferBase::operator bool() const
    {
        return m_storage != nullptr;
    }

    void BufferBase::release()
    {
        if (auto header = std::exchange(m_storage, nullptr))
        {
            if (header->refs.fetch_sub(1) == 1)
            {
                getBufferAllocator(*header).free(header);
            }
        }
    }

    bool BufferBase::sameBufferObject(const BufferBase& buffer_) const
    {
        return buffer_.m_storage && this->m_storage && (buffer_.m_storage == m_storage);
    }

    // bool BufferBase::sameBufferObject(const BufferView& view) const
    // {
    //     return view && sameBufferObject(view.underlyingBuffer());
    // }

    BufferBase::Header& BufferBase::header()
    {
        MY_DEBUG_CHECK(m_storage != nullptr);
        return *m_storage;
    }

    const BufferBase::Header& BufferBase::header() const
    {
        MY_DEBUG_CHECK(m_storage != nullptr);
        return *m_storage;
    }

    Buffer::Buffer() = default;

    Buffer::Buffer(size_t size)
    {
        m_storage = BufferStorage::allocate(size);
    }

    Buffer::Buffer(Buffer&& buffer) noexcept :
        BufferBase(buffer.m_storage)
    {
        buffer.m_storage = nullptr;
    }

    // Buffer::Buffer(BufferView&& buffer) noexcept
    // {
    //     this->operator=(std::move(buffer));
    // }

    Buffer& Buffer::operator=(Buffer&& buffer) noexcept
    {
        release();
        std::swap(buffer.m_storage, m_storage);
        return *this;
    }

#if 0
    Buffer& Buffer::operator=(BufferView&& bufferView) noexcept
    {
        scope_on_leave
        {
            bufferView = BufferView{};
        };

        this->release();

        if (const size_t viewSize = bufferView.size(); viewSize > 0)
        {
            if (BufferUtils::refsCount(bufferView) == 1 && bufferView.offset() == 0)
            {
                this->operator=(bufferView.m_buffer.toBuffer());
                this->resize(viewSize);
            }
            else
            {
                m_storage = BufferStorage::allocate(viewSize, bufferView.m_buffer.header().allocator);
                memcpy(this->data(), bufferView.data(), viewSize);
            }
        }

        return *this;
    }
#endif
    Buffer& Buffer::operator=(std::nullptr_t) noexcept
    {
        release();
        return *this;
    }

    void* Buffer::data() const
    {
        return clientData(m_storage);
    }

    void* Buffer::append(size_t count)
    {
        const size_t offset = size();
        resize(offset + count);
        return reinterpret_cast<std::byte*>(data()) + offset;
    }

    void Buffer::resize(size_t newSize)
    {
        if (m_storage == nullptr)
        {
            m_storage = BufferStorage::allocate(newSize);
            return;
        }

        MY_DEBUG_FATAL(refsCount(m_storage) == 1);
        m_storage = BufferStorage::reallocate(m_storage, newSize);
        

        // auto handle = BufferStorage::allocate(newSize);
        // MY_DEBUG_FATAL(handle);


        // if (const void* const ptr = data())
        // {
        //     //MY_DEBUG_CHECK(m_storage->size > 0);

        //     const size_t copySize = std::min(header().size, newSize);
        //     memcpy(clientData(storage), ptr, copySize);
        // }

        // this->release();
        // m_storage = storage;

        // //MY_DEBUG_CHECK(header().size == newSize);
    }

    ReadOnlyBuffer Buffer::toReadOnly()
    {
        return ReadOnlyBuffer{std::move(*this)};
    }

#if 0
    void Buffer::operator+=(BufferView&& source) noexcept
    {
        if (source.size() == 0)
        {
            return;
        }

        if (!m_storage)
        {
            *this = std::move(source);
        }
        else if (header().allocator == source.m_buffer.header().allocator && this->empty() && source.offset() == 0)
        {
            *this = std::move(source);
        }
        else
        {
            std::byte* const ptr = this->append(source.size());
            memcpy(ptr, source.data(), source.size());

            source.release();
        }
    }
#endif

    ReadOnlyBuffer::ReadOnlyBuffer() = default;

    ReadOnlyBuffer::ReadOnlyBuffer(Buffer&& buffer) noexcept
    {
        MY_DEBUG_CHECK(!buffer || BufferUtils::refsCount(buffer) == 1);
        *this = std::move(buffer);
    }

    ReadOnlyBuffer::ReadOnlyBuffer(const ReadOnlyBuffer& other) :
        BufferBase(other.m_storage)
    {
        if (m_storage)
        {
            MY_CHECK(m_storage->refs.fetch_add(1) > 0);
        }
    }

    ReadOnlyBuffer::ReadOnlyBuffer(ReadOnlyBuffer&& other) noexcept :
        BufferBase(other.m_storage)
    {
        other.m_storage = nullptr;
    }

    ReadOnlyBuffer& ReadOnlyBuffer::operator=(Buffer&& other) noexcept
    {
        this->release();
        std::swap(this->m_storage, other.m_storage);

        return *this;
    }

    ReadOnlyBuffer& ReadOnlyBuffer::operator=(const ReadOnlyBuffer& other)
    {
        release();
        if (m_storage = other.m_storage; m_storage)
        {
            MY_CHECK(m_storage->refs.fetch_add(1) > 0);
        }

        return *this;
    }

    ReadOnlyBuffer& ReadOnlyBuffer::operator=(ReadOnlyBuffer&& other) noexcept
    {
        this->release();
        std::swap(this->m_storage, other.m_storage);
        return *this;
    }

    ReadOnlyBuffer& ReadOnlyBuffer::operator=(std::nullptr_t) noexcept
    {
        this->release();
        return *this;
    }

    const void* ReadOnlyBuffer::data() const
    {
        return clientData(m_storage);
    }

    Buffer ReadOnlyBuffer::toBuffer()
    {
        if (!m_storage)
        {
            return {};
        }

        if (refsCount(m_storage) == 1)
        {
            BufferHeader* const storage = std::exchange(m_storage, nullptr);
            return Buffer{storage};
        }

        Buffer buffer = BufferUtils::copy(*this);
        release();
        return buffer;
    }
#if 0
    BufferView::BufferView() :
        m_offset(0),
        m_size(0)
    {
    }

    BufferView::BufferView(Buffer&& buffer) noexcept
        :
        m_buffer(buffer.toReadOnly()),
        m_offset(0)
    {
        m_size = m_buffer.size();
    }

    BufferView::BufferView(const ReadOnlyBuffer& buffer, size_t offset_, std::optional<size_t> size_) :
        m_buffer(buffer),
        m_offset(offset_),
        m_size(size_ ? *size_ : m_buffer.size() - offset_)
    {
        if (!m_buffer)
        {
            MY_DEBUG_CHECK(m_offset == 0 && (!size_ || *size_ == 0), "Invalid parameters while construct BufferView from empty ReadOnlyBuffer");
        }
        else
        {
            MY_DEBUG_CHECK(m_offset + m_size <= m_buffer.size());
        }
    }

    BufferView::BufferView(ReadOnlyBuffer&& buffer, size_t offset_, std::optional<size_t> size_) :
        m_buffer(std::move(buffer)),
        m_offset(offset_),
        m_size(size_ ? *size_ : m_buffer.size() - offset_)
    {
        if (!m_buffer)
        {
            MY_DEBUG_CHECK(m_offset == 0 && (!size_ || *size_ == 0), "Invalid parameters while construct BufferView from empty ReadOnlyBuffer");
        }
        else
        {
            MY_DEBUG_CHECK(m_offset + m_size <= m_buffer.size());
        }
    }

    BufferView::BufferView(const BufferView& buffer, size_t offset_, std::optional<size_t> size_) :
        m_buffer(buffer.m_buffer),
        m_offset(buffer.m_offset + offset_),
        m_size(size_ ? *size_ : buffer.m_size - offset_)
    {
        MY_DEBUG_CHECK((m_offset + m_size) <= m_buffer.size());
    }

    BufferView::BufferView(BufferView&& buffer, size_t offset_, std::optional<size_t> size_) :
        m_buffer(std::move(buffer.m_buffer)),
        m_offset(buffer.m_offset + offset_),
        m_size(size_ ? *size_ : buffer.m_size - offset_)
    {
        MY_DEBUG_CHECK((m_offset + m_size) <= m_buffer.size());
    }

    BufferView::BufferView(const BufferView&) = default;

    BufferView::BufferView(BufferView&& other) noexcept
        :
        m_buffer(std::move(other.m_buffer)),
        m_offset(other.m_offset),
        m_size(other.m_size)
    {
        other.m_offset = 0;
        other.m_size = 0;
    }

    BufferView& BufferView::operator=(const BufferView&) = default;

    BufferView& BufferView::operator=(BufferView&& other) noexcept
    {
        m_buffer = std::move(other.m_buffer);
        m_offset = other.m_offset;
        m_size = other.m_size;

        other.m_offset = 0;
        other.m_size = 0;

        return *this;
    }

    bool BufferView::operator==(const BufferView& other) const
    {
        return m_buffer.sameBufferObject(other.m_buffer) && m_size == other.m_size && m_offset == other.m_offset;
    }

    bool BufferView::operator!=(const BufferView& other) const
    {
        return !m_buffer.sameBufferObject(other.m_buffer) || m_size != other.m_size || m_offset != other.m_offset;
    }

    void BufferView::release()
    {
        if (m_buffer)
        {
            *this = BufferView{};
        }
    }

    const std::byte* BufferView::data() const
    {
        return m_buffer ? m_buffer.data() + m_offset : nullptr;
    }

    size_t BufferView::size() const
    {
        return m_size;
    }

    size_t BufferView::offset() const
    {
        return m_offset;
    }

    BufferView::operator bool() const
    {
        return static_cast<bool>(m_buffer);
    }

    ReadOnlyBuffer BufferView::underlyingBuffer() const
    {
        return m_buffer;
    }

    Buffer BufferView::toBuffer()
    {
        if (!m_buffer || m_size == 0)
        {
            return {};
        }

        scope_on_leave
        {
            if (m_buffer)
            {
                m_buffer.release();
            }
            m_size = 0;
            m_offset = 0;
        };

        if (m_offset == 0 && BufferUtils::refsCount(m_buffer) == 1)
        {
            Buffer buffer = m_buffer.toBuffer();
            buffer.resize(m_size);
            return buffer;
        }

        return BufferUtils::copy(*this);
    }
#endif

    // BufferView BufferView::merge(const BufferView& buffer) const
    //{
    //	return {};
    // }
    //
    // BufferView BufferView::merge(const BufferView& buffer1, const BufferView& buffer2)
    //{
    //	return {};
    // }

    uint32_t BufferUtils::refsCount(const BufferBase& buffer)
    {
        //return my::refsCount(buffer.m_storage);
        return my::refsCount(buffer.m_storage);
    }

    // uint32_t BufferUtils::refsCount(const BufferView& buffer)
    // {
    //     return BufferUtils::refsCount(buffer.m_buffer);
    // }

    Buffer BufferUtils::copy(const BufferBase& source, size_t offset, std::optional<size_t> size)
    {
        MY_DEBUG_CHECK(offset <= source.size());
        MY_DEBUG_CHECK(!size || offset + *size <= source.size());
        if (!source)
        {
            return {};
        }

        const size_t copySize = size.value_or(source.size() - offset);
        Buffer buffer{copySize};

        const std::byte* const src = clientData(source.m_storage);
        memcpy(buffer.data(), src + offset, copySize);
        return buffer;
    }

    // Buffer BufferUtils::copy(const BufferView& source, size_t offset, std::optional<size_t> size)
    // {
    //     MY_DEBUG_CHECK(offset <= source.size());
    //     MY_DEBUG_CHECK(!size || offset + *size <= source.size());

    //     if (!source)
    //     {
    //         return {};
    //     }

    //     const ReadOnlyBuffer& sourceBuffer = source.m_buffer;

    //     const size_t targetSize = size ? *size : (source.size() - offset);
    //     std::byte* const targetStorage = BufferStorage::allocate(targetSize, sourceBuffer.header().allocator);
    //     Buffer buffer{targetStorage};

    //     memcpy(buffer.data(), source.data() + offset, targetSize);
    //     return buffer;
    // }

}  // namespace my
