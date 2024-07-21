#pragma once
#include <FeCore/IO/IStream.h>
#include <FeCore/Console/FeLog.h>

namespace FE::IO
{
    class StreamBase : public IStream
    {
    public:
        FE_CLASS_RTTI(StreamBase, "2F74FF8D-4D81-44BE-962A-9D30669E03C8");

        ~StreamBase() override = default;

        inline USize WriteFromStream(IStream* stream, USize size) override
        {
            FE_ASSERT_MSG(stream, "Stream was nullptr");
            FE_ASSERT_MSG(stream->ReadAllowed(), "Source stream was write-only");
            FE_ASSERT_MSG(WriteAllowed(), "Destination stream was read-only");
            FE_ASSERT_MSG(stream != this, "Destination and source streams are the same");

            static constexpr USize tempSize = 128;
            Int8 temp[tempSize];
            USize result = 0;

            for (USize offset = 0; offset < size; offset += tempSize)
            {
                auto remaining = size - offset;
                auto currentSize = std::min(remaining, tempSize);
                stream->ReadToBuffer(temp, currentSize);
                result += WriteFromBuffer(temp, currentSize);
            }

            return result;
        }
    };

    class RStreamBase : public StreamBase
    {
    public:
        FE_CLASS_RTTI(RStreamBase, "339049F5-E6B9-481F-9AC5-4690EDDAF0F5");

        ~RStreamBase() override = default;

        [[nodiscard]] inline bool WriteAllowed() const noexcept override
        {
            return false;
        }

        [[nodiscard]] inline bool ReadAllowed() const noexcept override
        {
            return true;
        }

        inline USize WriteFromBuffer([[maybe_unused]] const void* buffer, [[maybe_unused]] USize size) override
        {
            FE_UNREACHABLE("Stream {} is read-only", GetName());
            return 0;
        }

        inline USize WriteFromStream([[maybe_unused]] IStream* stream, [[maybe_unused]] USize size) override
        {
            FE_UNREACHABLE("Stream {} is read-only", GetName());
            return 0;
        }

        [[nodiscard]] inline OpenMode GetOpenMode() const override
        {
            return OpenMode::ReadOnly;
        }
    };

    class WStreamBase : public StreamBase
    {
    public:
        FE_CLASS_RTTI(WStreamBase, "41056CA7-2943-4473-8041-3D4DB12619E3");

        [[nodiscard]] inline bool WriteAllowed() const noexcept override
        {
            return true;
        }

        [[nodiscard]] inline bool ReadAllowed() const noexcept override
        {
            return false;
        }

        [[nodiscard]] inline bool SeekAllowed() const noexcept override
        {
            return false;
        }

        inline ResultCode Seek([[maybe_unused]] SSize offset, [[maybe_unused]] SeekMode seekMode) override
        {
            FE_UNREACHABLE("Stream {} is write-only", GetName());
            return ResultCode::InvalidSeek;
        }

        [[nodiscard]] inline USize Tell() const override
        {
            return 0;
        }

        [[nodiscard]] inline USize Length() const override
        {
            return 0;
        }

        inline USize ReadToBuffer([[maybe_unused]] void* buffer, [[maybe_unused]] USize size) override
        {
            FE_UNREACHABLE("Stream {} is write-only", GetName());
            return 0;
        }

        [[nodiscard]] inline OpenMode GetOpenMode() const override
        {
            return OpenMode::CreateNew;
        }
    };
}
