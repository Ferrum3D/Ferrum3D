#pragma once
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <Core/Logging/Logger.h>
#include <Core/Math/Vector2.h>
#include <Graphics/Core/Format.h>

namespace FE::AssetBuilder
{
    struct TextureProcessSettings final
    {
        IO::IStreamFactory* m_streamFactory = nullptr;

        IO::Path m_inputFile;
        IO::Path m_outputFile;
        Vector2UInt m_outputSize = Vector2UInt::kZero;
        Graphics::Core::Format m_format = Graphics::Core::Format::kBC7_UNORM;
        bool m_generateMips = true;
    };


    bool ProcessTexture(const TextureProcessSettings& settings);
} // namespace FE::AssetBuilder
