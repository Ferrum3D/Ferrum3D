#pragma once
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>

namespace FE::Serialization
{
    Format DetectDocumentFormat(IO::IStream& stream, uint64_t expectedPackedHeader);


    template<class T>
    ResultCode LoadDocument(IO::IStream& stream, T& value)
    {
        switch (DetectDocumentFormat(stream, HashAll(Rtti::GetTypeID<T>(), GetVersion<T>(), GetSchemaHash<T>())))
        {
        case Format::kTaggedBinary:
            {
                TaggedBinaryFormat format;
                DeserializationContext context(&stream, format);
                return context.Load(value);
            }
        case Format::kPackedBinary:
            {
                PackedBinaryFormat format;
                DeserializationContext context(&stream, format);
                return context.Load(value);
            }
        default:
            {
                JsonFormat format;
                DeserializationContext context(&stream, format);
                return context.Load(value);
            }
        }
    }
} // namespace FE::Serialization
