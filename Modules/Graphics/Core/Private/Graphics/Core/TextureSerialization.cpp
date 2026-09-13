#include <Graphics/Core/Texture.h>

#include <Core/Serialization/Serialization.h>

namespace FE::Graphics::Core
{
    namespace
    {
        template<class T>
        struct NamedField final
        {
            festd::ascii_view m_name;
            T& m_value;
        };


        template<class T>
        NamedField<T> MakeField(const festd::ascii_view name, T& value)
        {
            return { name, value };
        }


        template<class TContext, class... TFields>
        Serialization::ResultCode TransferObject(TContext& context, TFields... fields)
        {
            if (auto object = context.BeginObject())
                (object.Field(fields.m_name, fields.m_value), ...);

            return context.GetResultCode();
        }
    } // namespace


    Serialization::ResultCode TextureDesc::Serialize(Serialization::SerializationContext& context, const TextureDesc& value)
    {
        const uint32_t width = value.m_width;
        const uint32_t height = value.m_height;
        const uint32_t sampleCount = value.m_sampleCount;
        const uint32_t depth = value.m_depth;
        const uint32_t arraySize = value.m_arraySize;
        const uint32_t mipSliceCount = value.m_mipSliceCount;
        const TextureDimension dimension = value.m_dimension;

        return TransferObject(context,
                              MakeField("m_width", width),
                              MakeField("m_height", height),
                              MakeField("m_sampleCount", sampleCount),
                              MakeField("m_depth", depth),
                              MakeField("m_arraySize", arraySize),
                              MakeField("m_mipSliceCount", mipSliceCount),
                              MakeField("m_dimension", dimension),
                              MakeField("m_imageFormat", value.m_imageFormat));
    }


    Serialization::ResultCode TextureDesc::Deserialize(Serialization::DeserializationContext& context, TextureDesc& value)
    {
        uint32_t width = value.m_width;
        uint32_t height = value.m_height;
        uint32_t sampleCount = value.m_sampleCount;
        uint32_t depth = value.m_depth;
        uint32_t arraySize = value.m_arraySize;
        uint32_t mipSliceCount = value.m_mipSliceCount;
        TextureDimension dimension = value.m_dimension;
        Format imageFormat = value.m_imageFormat;

        const Serialization::ResultCode result = TransferObject(context,
                                                                MakeField("m_width", width),
                                                                MakeField("m_height", height),
                                                                MakeField("m_sampleCount", sampleCount),
                                                                MakeField("m_depth", depth),
                                                                MakeField("m_arraySize", arraySize),
                                                                MakeField("m_mipSliceCount", mipSliceCount),
                                                                MakeField("m_dimension", dimension),
                                                                MakeField("m_imageFormat", imageFormat));
        if (result != Serialization::ResultCode::kSuccess)
            return result;

        value.m_width = width;
        value.m_height = height;
        value.m_sampleCount = sampleCount;
        value.m_depth = depth;
        value.m_arraySize = arraySize;
        value.m_mipSliceCount = mipSliceCount;
        value.m_dimension = dimension;
        value.m_imageFormat = imageFormat;
        return Serialization::ResultCode::kSuccess;
    }
} // namespace FE::Graphics::Core
