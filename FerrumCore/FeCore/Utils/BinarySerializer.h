#pragma once
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

        inline size_t Write(const void* data, size_t size)
        {
            return m_Stream->WriteFromBuffer(data, size);
        }

        template<class T>
        inline size_t Write(T value)
        {
            return m_Stream->WriteFromBuffer(&value, sizeof(T));
        }

        template<class T>
        inline size_t Read(T& value)
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
        inline size_t WriteArray(festd::span<T> array)
        {
            const auto length = array.size() * sizeof(T);
            Write<size_t>(array.size());
            return m_Stream->WriteFromBuffer(array.data(), length) + sizeof(size_t);
        }

        inline size_t WriteString(StringSlice string)
        {
            Write<size_t>(string.Size());
            return m_Stream->WriteFromBuffer(string.Data(), string.Size());
        }

        template<class T>
        inline size_t ReadArray(eastl::vector<T>& buffer)
        {
            buffer.resize(static_cast<uint32_t>(Read<size_t>()));
            return m_Stream->ReadToBuffer(buffer.data(), buffer.size() * sizeof(T)) + sizeof(size_t);
        }

        template<class T>
        inline eastl::vector<T> ReadArray()
        {
            eastl::vector<T> buffer;
            ReadArray<T>(buffer);
            return buffer;
        }

        inline size_t ReadString(String& string)
        {
            string = String(static_cast<uint32_t>(Read<size_t>()), '\0');
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
