#pragma once
#include <FeCore/Containers/IByteBuffer.h>
#include <FeCore/Containers/List.h>

namespace FE::GPU
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
} // namespace FE::GPU
