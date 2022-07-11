#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Containers/IByteBuffer.h>
#include <FeCore/Containers/List.h>

namespace FE::Osmium
{
    template<class T>
    inline void CopyFromByteBuffer(IByteBuffer* src, List<T>& dst)
    {
        if (src == nullptr)
        {
            return;
        }

        auto size = src->Size() / sizeof(T);
        if (size)
        {
            dst.Resize(size);
            memcpy(dst.Data(), src->Data(), src->Size());
            src->ReleaseStrongRef();
        }
    }

    template<class T>
    inline void ArraySliceFromByteBuffer(IByteBuffer* src, ArraySlice<T>& dst)
    {
        dst = ArraySlice<T>(reinterpret_cast<T*>(src->Data()), reinterpret_cast<T*>(src->Data() + src->Size()));
    }
} // namespace FE::Osmium
