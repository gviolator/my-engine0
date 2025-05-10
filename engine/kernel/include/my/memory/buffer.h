// #my_engine_source_file
#pragma once

#include <optional>
#include <string_view>

#include "my/kernel/kernel_config.h"

namespace my
{

    class BufferBase;
    class Buffer;
    class ReadOnlyBuffer;
    class BufferView;
    struct BufferStorage;
    struct BufferUtils;

    /**
     * @brief Converts BufferBase object to a string view.
     *
     * @param buffer The buffer to be converted.
     * @return A string view representing the buffer.
     */
    std::string_view asStringView(const BufferBase& buffer);

    /*
     *
     */
    Buffer fromStringView(std::string_view string);

    /**
     * @brief Base class for buffer management.
     */
    class MY_KERNEL_EXPORT BufferBase
    {
    public:
        struct Header;

        /**
         * @brief Gets the size of the buffer.
         *
         * @return The size of the buffer.
         */
        size_t size() const;

        /**
         * @brief Checks if the buffer is empty.
         *
         * @return True if the buffer is empty, false otherwise.
         */
        bool empty() const;

        /**
         * @brief Checks if the buffer is valid.
         *
         * @return True if the buffer is valid, false otherwise.
         */
        explicit operator bool() const;

        /**
         * @brief Releases the buffer.
         */
        void release();

        /**
         * @brief Checks if another buffer references the same memory.
         *
         * @param other The buffer to compare.
         * @return True if both buffers reference the same memory, false otherwise.
         */
        bool sameBufferObject(const BufferBase&) const;

        /**
         * @brief Checks if a BufferView references the same memory.
         *
         * @param view The buffer view to compare.
         * @return True if both reference the same memory, false otherwise.
         */
        //bool sameBufferObject(const BufferView&) const;

    protected:
        BufferBase();
        explicit BufferBase(Header*);
        ~BufferBase();
        Header& header();
        const Header& header() const;

        Header* m_storage;

        friend struct BufferStorage;
        friend struct BufferUtils;

        friend std::string_view asStringView(const BufferBase& buffer);
    };

    /**
     * @brief Class for managing mutable byte buffers.
     */
    class MY_KERNEL_EXPORT Buffer : public BufferBase
    {
    public:
        /**
         * @brief Default constructor.
         */
        Buffer();

        /**
         * @brief Constructs a buffer with the specified size.
         *
         * @param size The size of the buffer.
         * @param allocator Optional allocator for the buffer.
         */
        Buffer(size_t size);

        Buffer(const Buffer& buffer) = delete;

        /**
         * @brief Move constructor.
         *
         * @param buffer The buffer to move.
         */
        Buffer(Buffer&& buffer) noexcept;

        /**
         * @brief Constructs a buffer from a buffer view.
         *
         * @param buffer The buffer view to move.
         */
        //Buffer(BufferView&& buffer) noexcept;

        Buffer& operator=(const Buffer& buffer) = delete;

        /**
         * @brief Move assignment operator.
         *
         * @param buffer The buffer to move.
         * @return Reference to the current buffer.
         */
        Buffer& operator=(Buffer&& buffer) noexcept;

        /**
         * @brief Move assignment operator.
         *
         * @param buffer The buffer to move.
         * @return Reference to the current buffer.
         */
        Buffer& operator=(BufferView&& buffer) noexcept;

        Buffer& operator=(std::nullptr_t) noexcept;

        //void operator+=(BufferView&&) noexcept;

        /**
         * @brief Gets a pointer to the data in the buffer.
         *
         * @return Pointer to the data.
         */
         std::byte* data() const;

        /**
         * @brief Appends data to the buffer.
         *
         * @param size The size of the data to append.
         * @return Pointer to the appended data.
         */
        std::byte* append(size_t size);

        /**
         * @brief Resizes the buffer.
         *
         * @param newSize The new size of the buffer.
         */
        void resize(size_t);

        /**
         * @brief Converts the buffer to a read-only buffer.
         *
         * @return A read-only buffer.
         */
        ReadOnlyBuffer toReadOnly();

    private:
        using BufferBase::BufferBase;

        friend class ReadOnlyBuffer;
        friend struct BufferStorage;
    };

    /**
     * @brief Class for managing read-only byte buffers.
     */
    class MY_KERNEL_EXPORT ReadOnlyBuffer : public BufferBase
    {
    public:
        ReadOnlyBuffer();
        explicit ReadOnlyBuffer(Buffer&&) noexcept;
        ReadOnlyBuffer(const ReadOnlyBuffer&);
        ReadOnlyBuffer(ReadOnlyBuffer&&) noexcept;

        ReadOnlyBuffer& operator=(Buffer&&) noexcept;
        ReadOnlyBuffer& operator=(const ReadOnlyBuffer&);
        ReadOnlyBuffer& operator=(ReadOnlyBuffer&&) noexcept;
        ReadOnlyBuffer& operator=(std::nullptr_t) noexcept;

        /**
         * @brief Gets a pointer to the data in the buffer.
         *
         * @return Pointer to the data.
         */
        const std::byte* data() const;

        /**
         * @brief Converts the read-only buffer to a mutable buffer.
         *
         * @return A mutable buffer.
         */
        Buffer toBuffer();

        friend class Buffer;
    };
#if 0
    /**
     * @brief Class for managing views into buffers.
     */
    class MY_KERNEL_EXPORT BufferView
    {
    public:
        BufferView();
        BufferView(Buffer&&) noexcept;
        BufferView(const ReadOnlyBuffer&, size_t offset_ = 0, std::optional<size_t> size_ = std::nullopt);
        BufferView(ReadOnlyBuffer&&, size_t offset_ = 0, std::optional<size_t> size_ = std::nullopt);
        BufferView(const BufferView&, size_t offset_, std::optional<size_t> size_ = std::nullopt);
        BufferView(BufferView&&, size_t offset_, std::optional<size_t> size_ = std::nullopt);
        BufferView(const BufferView&);
        BufferView(BufferView&&) noexcept;

        BufferView& operator=(const BufferView&);

        BufferView& operator=(BufferView&&) noexcept;

        bool operator==(const BufferView&) const;

        bool operator!=(const BufferView&) const;

        /**
         * @brief Releases the buffer view.
         */
        void release();

        /**
         * @brief Gets a pointer to the data in the buffer view.
         *
         * @return Pointer to the data.
         */
        const std::byte* data() const;

        /**
         * @brief Gets the size of the buffer view.
         *
         * @return The size of the buffer view.
         */
        size_t size() const;

        /**
         * @brief Gets the offset of the buffer view.
         *
         * @return The offset of the buffer view.
         */
        size_t offset() const;

        /**
         * @brief Checks if the buffer view is valid.
         *
         * @return True if the buffer view is valid, false otherwise.
         */
        explicit operator bool() const;

        /**
         * @brief Get internally referenced buffer
         *
         * @return The underlying buffer.
         */
        ReadOnlyBuffer getUnderlyingBuffer() const;

        /**
         * @brief Converts the buffer view to a mutable buffer.
         *
         * @return A mutable buffer.
         */
        Buffer toBuffer();

        /**
            Merge this buffer with another one. This method does not modify buffer itself.
            Internally BufferView::merge used to produce result.
        */
        // BufferView merge(const BufferView& buffer) const;

        /// <summary>
        /// Merge buffers into the new one. If both buffers points to the same (internal) buffer a copy will not be created (only internal parameters will be adjusted).
        /// </summary>
        /// <param name="buffer1">First buffer to merge</param>
        /// <param name="buffer2">Second buffer to merge</param>
        /// <returns>Merged buffer.</returns>
        // static BufferView merge(const BufferView& buffer1, const BufferView& buffer2);

    private:
        ReadOnlyBuffer m_buffer;
        size_t m_offset;
        size_t m_size;

        friend struct BufferUtils;
        friend class Buffer;
    };
#endif

    using BufferHandle = BufferBase::Header*;

    /**
     */
    struct MY_KERNEL_EXPORT BufferStorage
    {
        /**
         */
        static BufferHandle allocate(size_t size);

        /**
         */
        static BufferHandle reallocate(BufferHandle buffer, size_t newSize);

        /**
         */
        static void release(BufferHandle& storage);

        /**
         */
        static BufferHandle takeOut(BufferBase&&);

        /**
         */
        static std::byte* getClientData(BufferHandle buffer);

        static size_t getClientSize(const BufferHandle buffer);

        static Buffer bufferFromHandle(BufferHandle buffer);

        static Buffer bufferFromClientData(std::byte* ptr, std::optional<size_t> size = std::nullopt);
    };

    /**
     * @brief Utility functions for buffer operations.
     */
    struct BufferUtils
    {
        /**
         * @brief Gets the reference count of a buffer.
         *
         * @param buffer The buffer.
         * @return The reference count.
         */
        MY_KERNEL_EXPORT
        static uint32_t refsCount(const BufferBase&);

        /**
         * @brief Gets the reference count of a buffer view.
         *
         * @param buffer The buffer view.
         * @return The reference count.
         */
        // MY_KERNEL_EXPORT
        // static uint32_t refsCount(const BufferView&);

        /**
         * @brief Copies data from a buffer.
         *
         * @param source The source buffer.
         * @param offset The offset in the source buffer.
         * @param size The size of the data to copy.
         * @return A new buffer containing the copied data.
         */
        MY_KERNEL_EXPORT
        static Buffer copy(const BufferBase&, size_t offset = 0, std::optional<size_t> size = std::nullopt);

        /**
         * @brief Copies data from a buffer view.
         *
         * @param source The source buffer view.
         * @param offset The offset in the source buffer view.
         * @param size The size of the data to copy.
         * @return A new buffer containing the copied data.
         */
        MY_KERNEL_EXPORT
        static Buffer copy(const BufferView&, size_t offset = 0, std::optional<size_t> size_ = std::nullopt);
    };

    inline std::string_view asStringView(const BufferBase& buffer)
    {
        if (!buffer || buffer.size() == 0)
        {
            return std::string_view{};
        }
        const std::byte* const data = BufferStorage::getClientData(buffer.m_storage);
        return std::string_view{reinterpret_cast<const char*>(data), buffer.size()};
    }

    inline Buffer fromStringView(std::string_view string)
    {
        Buffer buffer;
        if (!string.empty())
        {
            std::byte* const data = buffer.append(string.size());
            memcpy(data, string.data(), string.size());
        }
        return buffer;
    }
}  // namespace my
