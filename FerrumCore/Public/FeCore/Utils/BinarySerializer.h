#pragma once
#include <FeCore/IO/IStream.h>
#include <festd/string.h>

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

        size_t WriteString(const festd::string string)
        {
            Write<size_t>(string.size());
            return Write(string.data(), string.size());
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

        size_t ReadString(festd::string& string)
        {
            string = festd::string(static_cast<uint32_t>(Read<size_t>()), '\0');
            return m_stream->ReadToBuffer({ reinterpret_cast<std::byte*>(string.data()), string.size() });
        }

        festd::string ReadString()
        {
            festd::string string;
            ReadString(string);
            return string;
        }
    };
} // namespace FE
