#pragma once
#include <FeCore/IO/IStream.h>
#include <FeCore/Strings/String.h>

namespace FE
{
    class BinarySerializer final
    {
        Rc<IO::IStream> m_stream;

    public:
        explicit BinarySerializer(IO::IStream* stream)
            : m_stream(stream)
        {
        }

        size_t Write(const void* data, uint32_t size)
        {
            return m_stream->WriteFromBuffer({ static_cast<const std::byte*>(data), size });
        }

        template<class T>
        size_t Write(T value)
        {
            return Write(&value, sizeof(T));
        }

        template<class T>
        size_t Read(T& value)
        {
            return m_stream->ReadToBuffer({ reinterpret_cast<std::byte*>(&value), sizeof(T) });
        }

        template<class T>
        T Read()
        {
            T value;
            Read(value);
            return value;
        }

        template<class T>
        size_t WriteArray(festd::span<T> array)
        {
            const auto length = array.size() * sizeof(T);
            Write<size_t>(array.size());
            return m_stream->WriteFromBuffer(array.data(), length) + sizeof(size_t);
        }

        size_t WriteString(StringSlice string)
        {
            Write<size_t>(string.Size());
            return Write(string.Data(), string.Size());
        }

        template<class T>
        size_t ReadArray(eastl::vector<T>& buffer)
        {
            buffer.resize(static_cast<uint32_t>(Read<size_t>()));
            return m_stream->ReadToBuffer(buffer.data(), buffer.size() * sizeof(T)) + sizeof(size_t);
        }

        template<class T>
        eastl::vector<T> ReadArray()
        {
            eastl::vector<T> buffer;
            ReadArray<T>(buffer);
            return buffer;
        }

        size_t ReadString(String& string)
        {
            string = String(static_cast<uint32_t>(Read<size_t>()), '\0');
            return m_stream->ReadToBuffer({ reinterpret_cast<std::byte*>(string.Data()), string.Size() });
        }

        String ReadString()
        {
            String string;
            ReadString(string);
            return string;
        }
    };
} // namespace FE
