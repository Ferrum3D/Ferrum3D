#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Containers/List.h>

namespace FE::Osmium
{
    template<class T>
    inline void CopyFromByteBuffer(ByteBuffer& src, List<T>& dst)
    {
        auto size = src.Size() / sizeof(T);
        if (size)
        {
            dst.Resize(size);
            memcpy(dst.Data(), src.Data(), src.Size());
            src.Clear();
        }
    }

    template<class T>
    inline void ArraySliceFromByteBuffer(const ByteBuffer& src, ArraySlice<T>& dst)
    {
        dst = ArraySlice<T>(reinterpret_cast<const T*>(src.Data()), reinterpret_cast<const T*>(src.Data() + src.Size()));
    }

    template<class T>
    inline void ArraySliceFromByteBuffer(ByteBuffer& src, ArraySliceMut<T>& dst)
    {
        dst = ArraySliceMut<T>(reinterpret_cast<T*>(src.Data()), reinterpret_cast<T*>(src.Data() + src.Size()));
    }
} // namespace FE::Osmium
