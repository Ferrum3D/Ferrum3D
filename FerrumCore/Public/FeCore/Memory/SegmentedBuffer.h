#pragma once
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/SIMD/Utils.h>
#include <festd/vector.h>

namespace FE::Memory
{
    //! @brief A memory buffer that is split into segments.
    //!
    //! By design this is a POD type. It does not free memory on destruction.
    //! Use SegmentedBufferBuilder to create a SegmentedBuffer.
    struct SegmentedBuffer final
    {
        void Init()
        {
            m_allocator = nullptr;
            m_segments = nullptr;
            m_segmentCapacity = 0;
            m_segmentCount = 0;
        }

        void Free()
        {
            for (uint32_t i = 0; i < m_segmentCount; ++i)
                m_allocator->deallocate(m_segments[i], m_segmentCapacity);
            m_allocator->deallocate(static_cast<void*>(m_segments), m_segmentCount * sizeof(Segment*));

            Init();
        }

        struct alignas(kDefaultAlignment) Segment final
        {
            uint32_t m_size = 0;
        };

        static_assert(sizeof(Segment) == kDefaultAlignment);

        std::pmr::memory_resource* m_allocator;
        Segment** m_segments;
        uint32_t m_segmentCapacity;
        uint32_t m_segmentCount;
    };


    struct SegmentedBufferBuilder final
    {
        SegmentedBufferBuilder(std::pmr::memory_resource* allocator, const uint32_t segmentCapacity)
        {
            FE_Assert(IsAligned(segmentCapacity, SIMD::AVX::kByteSize));

            m_buffer.m_allocator = allocator;
            m_buffer.m_segmentCapacity = segmentCapacity;
            m_buffer.m_segments = nullptr;
            m_buffer.m_segmentCount = 0;
        }

        ~SegmentedBufferBuilder() = default;

        SegmentedBufferBuilder(const SegmentedBufferBuilder&) = delete;
        SegmentedBufferBuilder(SegmentedBufferBuilder&&) = delete;
        SegmentedBufferBuilder& operator=(const SegmentedBufferBuilder&) = delete;
        SegmentedBufferBuilder& operator=(SegmentedBufferBuilder&&) = delete;

        void* WriteBytes(const void* data, const uint32_t size)
        {
            SegmentedBuffer::Segment* segment;
            if (!m_segments.empty() && m_segments.back()->m_size + size <= m_buffer.m_segmentCapacity)
            {
                segment = m_segments.back();
            }
            else
            {
                segment = static_cast<SegmentedBuffer::Segment*>(m_buffer.m_allocator->allocate(m_buffer.m_segmentCapacity));
                SIMD::SSE::Zero(segment, sizeof(SegmentedBuffer::Segment));
                segment->m_size = sizeof(SegmentedBuffer::Segment);
                m_segments.push_back(segment);
            }

            void* result = reinterpret_cast<std::byte*>(segment) + segment->m_size;
            memcpy(result, data, size);
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
            return result;
        }

    private:
        festd::small_vector<SegmentedBuffer::Segment*> m_segments;
        SegmentedBuffer m_buffer;
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
                m_segmentOffset = sizeof(SegmentedBuffer::Segment);
                return ReadBytes(data, size);
            }

            memcpy(data, reinterpret_cast<const std::byte*>(segment) + m_segmentOffset, size);
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
        uint32_t m_segmentOffset = sizeof(SegmentedBuffer::Segment);
        SegmentedBuffer m_buffer;
    };
} // namespace FE::Memory
