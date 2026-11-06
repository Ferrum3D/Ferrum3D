#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)

    namespace DB
    {
        FE_CONSTEXPR uint32_t kTablePageSize = 64 * 1024;


        template<typename T>
        struct Ref
        {
            uint32_t m_rowIndex;

            void Invalidate()
            {
                m_rowIndex = kInvalidIndex;
            }

            static Ref CreateInvalid()
            {
                Ref value;
                value.Invalidate();
                return value;
            }
        };


        //! A field of shader data that holds the address of a table; shaders call its ReadRow directly.
#if FE_HOST
        template<class T>
        struct TableAddress final
        {
            BufferPointer m_pageTableAddress{ UINT64_C(0) };
        };
#else
        template<typename T>
        using TableAddress = T;
#endif


        template<typename T>
        struct Slice
        {
            uint32_t m_rowIndex;
            uint32_t m_count;

            void Invalidate()
            {
                m_rowIndex = kInvalidIndex;
                m_count = 0;
            }

            static Slice CreateInvalid()
            {
                Slice value;
                value.Invalidate();
                return value;
            }

            Ref<T> At(const uint32_t index) FE_CONST
            {
                Ref<T> row;
                row.m_rowIndex = m_rowIndex + index;
                return row;
            }
        };
    } // namespace DB

FE_HOST_END_NAMESPACE
