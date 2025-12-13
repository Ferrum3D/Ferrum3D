#include <Graphics/Core/Format.h>

namespace FE::Graphics
{
    Core::VertexChannelFormat Core::TranslateFormat(const Format format)
    {
        switch (format)
        {
        case Format::kUndefined:
            return VertexChannelFormat::kUndefined;

#define FE_TRANSLATE_FORMAT_TO_VERTEX_CHANNEL_FORMAT_CASE(name)                                                                  \
    case Format::k##name:                                                                                                        \
        return VertexChannelFormat::k##name;

            FE_EXPAND_VERTEX_CHANNEL_FORMATS(FE_TRANSLATE_FORMAT_TO_VERTEX_CHANNEL_FORMAT_CASE)

#undef FE_TRANSLATE_FORMAT_TO_VERTEX_CHANNEL_FORMAT_CASE

        default:
            // The specified format is not supported as a vertex channel format.
            FE_DebugBreak();
            return VertexChannelFormat::kUndefined;
        }
    }


    Core::Format Core::TranslateFormat(const VertexChannelFormat format)
    {
        switch (format)
        {
        case VertexChannelFormat::kUndefined:
            return Format::kUndefined;

#define FE_TRANSLATE_VERTEX_CHANNEL_FORMAT_TO_FORMAT_CASE(name)                                                                  \
    case VertexChannelFormat::k##name:                                                                                           \
        return Format::k##name;

            FE_EXPAND_VERTEX_CHANNEL_FORMATS(FE_TRANSLATE_VERTEX_CHANNEL_FORMAT_TO_FORMAT_CASE)

#undef FE_TRANSLATE_VERTEX_CHANNEL_FORMAT_TO_FORMAT_CASE

        default:
            FE_DebugBreak();
            return Format::kUndefined;
        }
    }


    festd::string_view Core::ToString(const Format format)
    {
        switch (format)
        {
#define FE_DECL_FORMAT_NAME(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                              \
    case Format::k##name:                                                                                                        \
        return #name;

            FE_EXPAND_FORMATS(FE_DECL_FORMAT_NAME)

        case Format::kUndefined:
            return "<undefined>";

        default:
            return "<unknown>";
        }
    }
} // namespace FE::Graphics
