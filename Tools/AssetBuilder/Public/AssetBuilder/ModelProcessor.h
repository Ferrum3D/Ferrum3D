#pragma once
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <Core/Logging/Logger.h>

namespace FE::AssetBuilder
{
    struct ModelProcessSettings final
    {
        IO::IStreamFactory* m_streamFactory = nullptr;
        Logger* m_logger = nullptr;

        IO::Path m_inputFile;
        IO::Path m_outputFile;
    };

    bool ProcessModel(const ModelProcessSettings& settings);
} // namespace FE::AssetBuilder
