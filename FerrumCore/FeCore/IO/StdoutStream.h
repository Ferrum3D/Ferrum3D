#pragma once
#include <FeCore/Console/Console.h>
#include <FeCore/IO/StreamBase.h>

namespace FE::IO
{
    class StdoutStream : public WStreamBase
    {
    public:
        FE_RTTI_Class(StdoutStream, "2D5441F8-10B1-4358-B486-5C6BF02DDB24");

        [[nodiscard]] bool IsOpen() const override;

        size_t WriteFromBuffer(const void* buffer, size_t size) override;

        StringSlice GetName() override;

        void Close() override;
    };
} // namespace FE::IO
