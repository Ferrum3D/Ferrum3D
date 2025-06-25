#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE::Trace
{
    struct SymbolInfo final
    {
        uintptr_t m_address = 0;
        uint32_t m_lineNumber = 0;
        char m_moduleName[32] = {};
        char m_fileName[260] = {};
        char m_symbolName[256] = {};

        [[nodiscard]] static SymbolInfo Resolve(void* ptr);
    };


    struct CallStack final : public TypedHandle<CallStack, uint32_t>
    {
        static constexpr uint32_t kDefaultMaxFrames = 64;
        static constexpr uint32_t kDefaultSkipFrames = 2;

        [[nodiscard]] static FE_FORCE_NOINLINE CallStack Capture(uint32_t maxFrames = kDefaultMaxFrames,
                                                                 uint32_t skipFrames = kDefaultSkipFrames);

        [[nodiscard]] void** GetFrames() const;
    };
} // namespace FE::Trace
