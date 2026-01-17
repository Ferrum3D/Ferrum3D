#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)

    namespace DB
    {
        FE_CONSTEXPR uint32_t kTablePageSize = 64 * 1024;
        FE_CONSTEXPR uint32_t kMaxTablePageCount = kTablePageSize / sizeof(BufferPointer);


        template<typename T>
        struct Ref
        {
            uint32_t m_rowIndex;
        };


        template<typename T>
        struct Slice
        {
            uint32_t m_rowIndex;
            uint32_t m_count;
        };
    } // namespace DB

FE_HOST_END_NAMESPACE
