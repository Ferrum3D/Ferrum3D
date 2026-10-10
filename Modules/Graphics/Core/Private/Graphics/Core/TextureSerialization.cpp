#include <Graphics/Core/Texture.h>

#include <Core/Serialization/Serialization.h>

namespace FE::Graphics::Core
{
    Serialization::ResultCode TextureDesc::Serialize(Serialization::SerializationContext& context, const TextureDesc& value)
    {
        const uint32_t width = value.m_width;
        const uint32_t height = value.m_height;
        const uint32_t sampleCount = value.m_sampleCount;
        const uint32_t depth = value.m_depth;
        const uint32_t arraySize = value.m_arraySize;
        const uint32_t mipSliceCount = value.m_mipSliceCount;
        const TextureDimension dimension = value.m_dimension;

        if (auto object = context.BeginObject())
        {
            object.Field("m_width", width);
            object.Field("m_height", height);
            object.Field("m_sampleCount", sampleCount);
            object.Field("m_depth", depth);
            object.Field("m_arraySize", arraySize);
            object.Field("m_mipSliceCount", mipSliceCount);
            object.Field("m_dimension", dimension);
            object.Field("m_imageFormat", value.m_imageFormat);
        }

        return context.GetResultCode();
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

        if (auto object = context.BeginObject())
        {
            object.Field("m_width", width);
            object.Field("m_height", height);
            object.Field("m_sampleCount", sampleCount);
            object.Field("m_depth", depth);
            object.Field("m_arraySize", arraySize);
            object.Field("m_mipSliceCount", mipSliceCount);
            object.Field("m_dimension", dimension);
            object.Field("m_imageFormat", imageFormat);
        }

        const Serialization::ResultCode result = context.GetResultCode();
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
