#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/IO/IStream.h>

namespace FE
{
    class BinarySerializer final
    {
        Shared<IO::IStream> m_Stream;

    public:
        inline explicit BinarySerializer(IO::IStream* stream)
            : m_Stream(stream)
        {
        }

        inline USize Write(const void* data, USize size)
        {
            return m_Stream->WriteFromBuffer(data, size);
        }

        template<class T>
        inline USize Write(T value)
        {
            return m_Stream->WriteFromBuffer(&value, sizeof(T));
        }

        template<class T>
        inline USize Read(T& value)
        {
            return m_Stream->ReadToBuffer(&value, sizeof(T));
        }

        template<class T>
        inline T Read()
        {
            T value;
            Read(value);
            return value;
        }

        template<class T>
        inline USize WriteArray(const ArraySlice<T>& array)
        {
            auto length = array.Length() * sizeof(T);
            Write<USize>(array.Length());
            return m_Stream->WriteFromBuffer(array.Data(), length) + sizeof(USize);
        }

        template<class T>
        inline USize ReadArray(List<T>& buffer)
        {
            buffer.Resize(Read<USize>());
            return m_Stream->ReadToBuffer(buffer.Data(), buffer.Size() * sizeof(T)) + sizeof(USize);
        }

        template<class T>
        inline List<T> ReadArray()
        {
            List<T> buffer;
            ReadArray<T>(buffer);
            return buffer;
        }
    };
} // namespace FE
