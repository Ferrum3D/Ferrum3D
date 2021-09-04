#pragma once
#include <FeCore/Console/Console.h>
#include <FeCore/IO/StreamBase.h>

namespace FE::IO
{
    class StdoutStream : public WStreamBase
    {
    public:
        FE_CLASS_RTTI(StdoutStream, "2D5441F8-10B1-4358-B486-5C6BF02DDB24");

        [[nodiscard]] bool IsOpen() const override;

        USize WriteFromBuffer(const void* buffer, USize size) override;

        StringSlice GetName() override;

        void Close() override;
    };
} // namespace FE::IO
