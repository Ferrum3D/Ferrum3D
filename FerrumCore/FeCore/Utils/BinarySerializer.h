#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/IO/IStream.h>
#include <FeCore/Strings/String.h>

namespace FE
{
    class BinarySerializer final
    {
        Rc<IO::IStream> m_Stream;

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

        inline USize WriteString(StringSlice string)
        {
            Write<USize>(string.Size());
            return m_Stream->WriteFromBuffer(string.Data(), string.Size());
        }

        template<class T>
        inline USize ReadArray(eastl::vector<T>& buffer)
        {
            buffer.resize(static_cast<uint32_t>(Read<USize>()));
            return m_Stream->ReadToBuffer(buffer.data(), buffer.size() * sizeof(T)) + sizeof(USize);
        }

        template<class T>
        inline eastl::vector<T> ReadArray()
        {
            eastl::vector<T> buffer;
            ReadArray<T>(buffer);
            return buffer;
        }

        inline USize ReadString(String& string)
        {
            string = String(Read<USize>(), '\0');
            return m_Stream->ReadToBuffer(string.Data(), string.Size());
        }

        inline String ReadString()
        {
            String string;
            ReadString(string);
            return string;
        }
    };
} // namespace FE
