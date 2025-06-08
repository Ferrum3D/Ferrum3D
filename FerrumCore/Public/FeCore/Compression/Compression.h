#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Base/BaseTypes.h>
#include <festd/base.h>

namespace FE::Compression
{
    enum class Method : uint8_t
    {
        kNone,
        kDeflate,
        kGDeflate,
        kInvalid,
    };


    struct BlockHeader final
    {
        uint32_t m_magic;
        uint32_t m_uncompressedPageSize;
    };


    struct PageHeader final
    {
        uint32_t m_compressedSize;
        uint32_t m_nextPageOffset;
    };


    struct BlockFooter final
    {
        uint32_t m_tailPageUncompressedSize;
        uint32_t m_crc32;
    };


    inline Method DecodeMagic(const uint32_t magic)
    {
        const uint32_t signature = magic & 0x00ffffff;
        if (signature != Math::MakeFourCC('F', 'C', 'B', 0))
            return Method::kInvalid;

        const Method method = static_cast<Method>(magic >> 24);
        if (method >= Method::kInvalid)
            return Method::kInvalid;

        return method;
    }


    struct Context final
    {
        Context() = default;
        ~Context();

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        Context(Context&& other) noexcept
        {
            std::swap(m_method, other.m_method);
            std::swap(m_level, other.m_level);
            std::swap(m_impl, other.m_impl);
        }

        Context& operator=(Context&& other) noexcept
        {
            std::swap(m_method, other.m_method);
            std::swap(m_level, other.m_level);
            std::swap(m_impl, other.m_impl);
            return *this;
        }

        void Reset();

        [[nodiscard]] size_t GetBounds(size_t uncompressedSize) const;

        [[nodiscard]] bool Compress(const void* src, size_t srcSize, void* dst, size_t dstSize) const;

        static Context Create(Method method, int32_t level = 6);

    private:
        Context(const Method method, const int32_t level, void* impl)
            : m_method(method)
            , m_level(static_cast<int8_t>(level))
            , m_impl(impl)
        {
        }

        Method m_method = Method::kNone;
        int8_t m_level = 0;
        void* m_impl = nullptr;
    };
} // namespace FE::Compression
