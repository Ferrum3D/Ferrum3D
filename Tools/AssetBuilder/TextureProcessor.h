#pragma once
#include <FeCore/IO/BaseIO.h>
#include <FeCore/IO/Path.h>
#include <FeCore/Logging/Logger.h>
#include <FeCore/Math/Vector2.h>
#include <Graphics/Core/ImageFormat.h>

namespace FE::AssetBuilder
{
    struct TextureProcessSettings final
    {
        IO::IStreamFactory* m_streamFactory = nullptr;
        Logger* m_logger = nullptr;

        IO::Path m_inputFile;
        IO::Path m_outputFile;
        Vector2UInt m_outputSize = Vector2UInt::Zero();
        Graphics::Core::Format m_format = Graphics::Core::Format::kBC7_UNORM;
        bool m_generateMips = true;
    };


    bool ProcessTexture(const TextureProcessSettings& settings);
} // namespace FE::AssetBuilder
