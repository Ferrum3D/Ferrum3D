#pragma once
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/SIMD/Utils.h>
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
            for (uint32_t i = 0; i < m_segmentCount; ++i)
                m_allocator->deallocate(m_segments[i], m_segments[i]->m_capacity);
            m_allocator->deallocate(static_cast<void*>(m_segments), m_segmentCount * sizeof(Segment*));

            Init();
        }

        struct alignas(kDefaultAlignment) Segment final
        {
            uint32_t m_size = 0;     //!< The number of bytes written to this segment.
            uint32_t m_capacity = 0; //!< The number of bytes allocated for this segment including the header.
        };

        static_assert(sizeof(Segment) == kDefaultAlignment);

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
            auto* segment =
                static_cast<SegmentedBuffer::Segment*>(m_buffer.m_allocator->allocate(size + sizeof(SegmentedBuffer::Segment)));
            Zero(segment, sizeof(SegmentedBuffer::Segment));
            segment->m_size = size;
            segment->m_capacity = size + sizeof(SegmentedBuffer::Segment);
            m_segments.push_back(segment);
            return reinterpret_cast<std::byte*>(segment + 1);
        }

        [[nodiscard]] festd::span<SegmentedBuffer::Segment* const> GetSegments() const
        {
            return { m_segments.data(), m_segments.size() };
        }

        SegmentedBuffer Build()
        {
            auto** segments = static_cast<SegmentedBuffer::Segment**>(
                m_buffer.m_allocator->allocate(m_segments.size() * sizeof(SegmentedBuffer::Segment*)));
            memcpy(static_cast<void*>(segments),
                   static_cast<const void*>(m_segments.data()),
                   m_segments.size() * sizeof(SegmentedBuffer::Segment*));
            m_buffer.m_segments = segments;
            m_buffer.m_segmentCount = m_segments.size();

            const SegmentedBuffer result = m_buffer;
            m_buffer.Init();
            m_segments.clear();
            return result;
        }

    private:
        festd::inline_vector<SegmentedBuffer::Segment*> m_segments;
        SegmentedBuffer m_buffer;
    };


    struct SegmentedBufferBuilder final
    {
        explicit SegmentedBufferBuilder(std::pmr::memory_resource* allocator, const uint32_t segmentCapacity)
        {
            FE_Assert(IsAligned(segmentCapacity, SIMD::AVX::kByteSize));

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
            void* ptr = Allocate(size);
            memcpy(ptr, data, size);
            return ptr;
        }

        void* Allocate(const uint32_t size)
        {
            SegmentedBuffer::Segment* segment;
            if (!m_segments.empty() && m_segments.back()->m_size + size + sizeof(SegmentedBuffer::Segment) <= m_segmentCapacity)
            {
                segment = m_segments.back();
            }
            else
            {
                segment = static_cast<SegmentedBuffer::Segment*>(m_buffer.m_allocator->allocate(m_segmentCapacity));
                Zero(segment, sizeof(SegmentedBuffer::Segment));
                segment->m_size = 0;
                segment->m_capacity = m_segmentCapacity;
                m_segments.push_back(segment);
            }

            void* result = reinterpret_cast<std::byte*>(segment + 1) + segment->m_size;
            segment->m_size += size;
            return result;
        }

        SegmentedBuffer Build()
        {
            auto** segments = static_cast<SegmentedBuffer::Segment**>(
                m_buffer.m_allocator->allocate(m_segments.size() * sizeof(SegmentedBuffer::Segment*)));
            memcpy(static_cast<void*>(segments),
                   static_cast<const void*>(m_segments.data()),
                   m_segments.size() * sizeof(SegmentedBuffer::Segment*));
            m_buffer.m_segments = segments;
            m_buffer.m_segmentCount = m_segments.size();

            const SegmentedBuffer result = m_buffer;
            m_buffer.Init();
            m_segments.clear();
            m_segmentCapacity = 0;
            return result;
        }

    private:
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

        [[nodiscard]] bool ReadBytes(void* data, const uint32_t size)
        {
            if (m_segmentIndex >= m_buffer.m_segmentCount)
                return false;

            const SegmentedBuffer::Segment* segment = m_buffer.m_segments[m_segmentIndex];
            if (m_segmentOffset + size > segment->m_size)
            {
                m_segmentIndex++;
                m_segmentOffset = 0;
                return ReadBytes(data, size);
            }

            memcpy(data, reinterpret_cast<const std::byte*>(segment + 1) + m_segmentOffset, size);
            m_segmentOffset += size;
            return true;
        }

        [[nodiscard]] bool SkipBytes(const uint32_t size)
        {
            if (m_segmentIndex >= m_buffer.m_segmentCount)
                return false;

            const SegmentedBuffer::Segment* segment = m_buffer.m_segments[m_segmentIndex];
            if (m_segmentOffset + size > segment->m_size)
            {
                m_segmentIndex++;
                m_segmentOffset = 0;
                return SkipBytes(size);
            }

            m_segmentOffset += size;
            return true;
        }

        [[nodiscard]] bool ReadBytesNoConsume(void* data, const uint32_t size) const
        {
            auto tempReader = *this;
            return tempReader.ReadBytes(data, size);
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

        uint32_t m_segmentIndex = 0;
        uint32_t m_segmentOffset = 0;
        SegmentedBuffer m_buffer;
    };
} // namespace FE::Memory
