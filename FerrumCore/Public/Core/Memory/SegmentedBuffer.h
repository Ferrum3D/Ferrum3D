#pragma once
#include <Core/Base/BaseTypes.h>
#include <Core/SIMD/Utils.h>
#include <festd/vector.h>

namespace FE::Memory
{
    //! @brief A memory buffer that is split into segments.
    //!
    //! By design this is a POD type. It does not free memory on destruction.
    //! Use SegmentedBufferBuilder or SegmentedBufferManualBuilder to create a SegmentedBuffer.
    struct SegmentedBuffer final
    {
        void Init()
        {
            m_allocator = nullptr;
            m_segments = nullptr;
            m_segmentCount = 0;
        }

        void Free()
        {
            if (m_segments != nullptr)
            {
                const bool isSmallTable = m_segments == &m_segments[0]->m_smallTable;
                for (uint32_t i = 0; i < m_segmentCount; ++i)
                    m_allocator->deallocate(m_segments[i], m_segments[i]->m_capacity);

                if (!isSmallTable)
                    m_allocator->deallocate(static_cast<void*>(m_segments), m_segmentCount * sizeof(Segment*));
            }

            Init();
        }

        struct alignas(kDefaultAlignment) Segment final
        {
            uint32_t m_size = 0;             //!< The number of bytes written to this segment.
            uint32_t m_capacity = 0;         //!< The number of bytes allocated for this segment including the header.
            Segment* m_smallTable = nullptr; //!< Segment table for single-segment buffers.
        };

        static_assert(sizeof(Segment) == kDefaultAlignment);

        static SegmentedBuffer Create(std::pmr::memory_resource* allocator, const festd::span<Segment*> segments)
        {
            SegmentedBuffer buffer;
            buffer.m_allocator = allocator;
            buffer.m_segmentCount = segments.size();
            if (segments.empty())
            {
                buffer.m_segments = nullptr;
            }
            else if (segments.size() == 1)
            {
                auto** segmentTable = &segments[0]->m_smallTable;
                segmentTable[0] = segments[0];
                buffer.m_segments = segmentTable;
            }
            else
            {
                auto** segmentTable = Memory::AllocateArray<Segment*>(allocator, segments.size());
                memcpy(static_cast<void*>(segmentTable),
                       static_cast<const void*>(segments.data()),
                       segments.size() * sizeof(Segment*));
                buffer.m_segments = segmentTable;
            }

            return buffer;
        }

        std::pmr::memory_resource* m_allocator;
        Segment** m_segments;
        uint32_t m_segmentCount;
    };


    struct SegmentedBufferManualBuilder final
    {
        explicit SegmentedBufferManualBuilder(std::pmr::memory_resource* allocator)
        {
            m_buffer.m_allocator = allocator;
            m_buffer.m_segments = nullptr;
            m_buffer.m_segmentCount = 0;
        }

        ~SegmentedBufferManualBuilder() = default;

        SegmentedBufferManualBuilder(const SegmentedBufferManualBuilder&) = delete;
        SegmentedBufferManualBuilder(SegmentedBufferManualBuilder&&) = delete;
        SegmentedBufferManualBuilder& operator=(const SegmentedBufferManualBuilder&) = delete;
        SegmentedBufferManualBuilder& operator=(SegmentedBufferManualBuilder&&) = delete;

        std::byte* AllocateSegment(const uint32_t size)
        {
            const uint32_t allocationSize = size + sizeof(SegmentedBuffer::Segment);
            auto* segment = new (m_buffer.m_allocator->allocate(allocationSize)) SegmentedBuffer::Segment;
            segment->m_size = size;
            segment->m_capacity = allocationSize;
            m_segments.push_back(segment);
            return reinterpret_cast<std::byte*>(segment + 1);
        }

        [[nodiscard]] festd::span<SegmentedBuffer::Segment* const> GetSegments() const
        {
            return { m_segments.data(), m_segments.size() };
        }

        SegmentedBuffer Build()
        {
            const SegmentedBuffer buffer = SegmentedBuffer::Create(m_buffer.m_allocator, m_segments);
            m_segments.clear();
            m_buffer.Init();
            return buffer;
        }

    private:
        festd::inline_vector<SegmentedBuffer::Segment*> m_segments;
        SegmentedBuffer m_buffer;
    };


    struct SegmentedBufferBuilder final
    {
        explicit SegmentedBufferBuilder(std::pmr::memory_resource* allocator, const uint32_t segmentCapacity)
        {
            FE_Assert(IsAligned(segmentCapacity, Simd::AVX::kByteSize));

            m_buffer.m_allocator = allocator;
            m_buffer.m_segments = nullptr;
            m_buffer.m_segmentCount = 0;
            m_segmentCapacity = segmentCapacity;
        }

        ~SegmentedBufferBuilder() = default;

        SegmentedBufferBuilder(const SegmentedBufferBuilder&) = delete;
        SegmentedBufferBuilder(SegmentedBufferBuilder&&) = delete;
        SegmentedBufferBuilder& operator=(const SegmentedBufferBuilder&) = delete;
        SegmentedBufferBuilder& operator=(SegmentedBufferBuilder&&) = delete;

        void* WriteBytes(const void* data, const uint32_t size)
        {
            void* ptr = Allocate(size, 1);
            memcpy(ptr, data, size);
            return ptr;
        }

        void* Allocate(const size_t byteSize, const size_t byteAlignment)
        {
            const uint32_t size = static_cast<uint32_t>(byteSize);
            const uint32_t alignment = Math::Max(static_cast<uint32_t>(byteAlignment), 1u);

            if (!m_segments.empty())
            {
                if (void* ptr = TryAllocateFromSegment(m_segments.back(), size, alignment))
                    return ptr;
            }

            auto* segment = new (m_buffer.m_allocator->allocate(m_segmentCapacity)) SegmentedBuffer::Segment;
            segment->m_size = 0;
            segment->m_capacity = m_segmentCapacity;
            m_segments.push_back(segment);

            if (void* ptr = TryAllocateFromSegment(segment, size, alignment))
                return ptr;

            FE_DebugBreak();
            return nullptr;
        }

        SegmentedBuffer ShrinkAndBuild(std::pmr::memory_resource* allocator = nullptr)
        {
            if (!m_segments.empty())
                return Build();

            if (allocator == nullptr)
                allocator = m_buffer.m_allocator;

            FE_Assert(m_segments.size() == 1);
            auto* oldSegment = m_segments.front();
            const uint32_t newCapacity = oldSegment->m_size + sizeof(SegmentedBuffer::Segment);
            auto* newSegment = new (allocator->allocate(newCapacity)) SegmentedBuffer::Segment;
            memcpy(newSegment, oldSegment, newCapacity);
            newSegment->m_capacity = newCapacity;
            m_buffer.m_allocator->deallocate(oldSegment, oldSegment->m_capacity);

            m_segments[0] = newSegment;
            m_buffer.m_allocator = allocator;
            return Build();
        }

        SegmentedBuffer Build()
        {
            const SegmentedBuffer buffer = SegmentedBuffer::Create(m_buffer.m_allocator, m_segments);
            m_segments.clear();
            m_buffer.Init();
            m_segmentCapacity = 0;
            return buffer;
        }

    private:
        void* TryAllocateFromSegment(SegmentedBuffer::Segment* segment, const uint32_t size, const uint32_t alignment) const
        {
            auto* segmentDataPtr = reinterpret_cast<std::byte*>(segment + 1);
            auto* ptr = AlignUpPtr(segmentDataPtr + segment->m_size, alignment);
            const uint32_t newSegmentSize = static_cast<uint32_t>(ptr - segmentDataPtr) + size;
            if (newSegmentSize + sizeof(SegmentedBuffer::Segment) <= m_segmentCapacity)
            {
                segment->m_size = newSegmentSize;
                return ptr;
            }

            return nullptr;
        }

        festd::inline_vector<SegmentedBuffer::Segment*> m_segments;
        SegmentedBuffer m_buffer;
        uint32_t m_segmentCapacity = 0;
    };


    struct SegmentedBufferReader final
    {
        explicit SegmentedBufferReader(const SegmentedBuffer& buffer)
            : m_buffer(buffer)
        {
        }

        [[nodiscard]] bool ReadBytes(void* data, const uint32_t size, const uint32_t alignment = 1)
        {
            if (!FindNextPosition(size, alignment, m_segmentIndex, m_segmentOffset))
                return false;

            SegmentedBuffer::Segment* segment = m_buffer.m_segments[m_segmentIndex];
            memcpy(data, reinterpret_cast<const std::byte*>(segment + 1) + m_segmentOffset, size);
            m_segmentOffset += size;
            return true;
        }

        [[nodiscard]] bool SkipBytes(const uint32_t size, const uint32_t alignment = 1)
        {
            if (!FindNextPosition(size, alignment, m_segmentIndex, m_segmentOffset))
                return false;

            m_segmentOffset += size;
            return true;
        }

        [[nodiscard]] bool ReadBytesNoConsume(void* data, const uint32_t size, const uint32_t alignment = 1) const
        {
            uint32_t segmentIndex = m_segmentIndex;
            uint32_t segmentOffset = m_segmentOffset;
            if (!FindNextPosition(size, alignment, segmentIndex, segmentOffset))
                return false;

            SegmentedBuffer::Segment* segment = m_buffer.m_segments[segmentIndex];
            memcpy(data, reinterpret_cast<const std::byte*>(segment + 1) + segmentOffset, size);
            return true;
        }

        template<class T>
        [[nodiscard]] bool Read(T& value)
        {
            return ReadBytes(&value, sizeof(T));
        }

        template<class T>
        [[nodiscard]] bool ReadNoConsume(T& value) const
        {
            return ReadBytesNoConsume(&value, sizeof(T));
        }

    private:
        static bool FindOffsetInSegment(const SegmentedBuffer::Segment* segment, const uint32_t size, const uint32_t alignment,
                                        uint32_t& segmentOffset)
        {
            const auto* segmentDataPtr = reinterpret_cast<const std::byte*>(segment + 1);
            const auto* ptr = AlignUpPtr(segmentDataPtr + segmentOffset, alignment);
            const uint32_t newSegmentOffset = static_cast<uint32_t>(ptr - segmentDataPtr) + size;
            if (newSegmentOffset <= segment->m_size)
            {
                segmentOffset = static_cast<uint32_t>(ptr - segmentDataPtr);
                return true;
            }

            return false;
        }

        bool FindNextPosition(const uint32_t size, const uint32_t alignment, uint32_t& segmentIndex,
                              uint32_t& segmentOffset) const
        {
            if (FindOffsetInSegment(m_buffer.m_segments[segmentIndex], size, alignment, segmentOffset))
                return true;

            if (segmentIndex + 1 >= m_buffer.m_segmentCount)
                return false;

            ++segmentIndex;
            segmentOffset = 0;
            return FindOffsetInSegment(m_buffer.m_segments[segmentIndex], size, alignment, segmentOffset);
        }

        uint32_t m_segmentIndex = 0;
        uint32_t m_segmentOffset = 0;
        SegmentedBuffer m_buffer;
    };
} // namespace FE::Memory
