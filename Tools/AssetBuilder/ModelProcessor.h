#pragma once
#include <FeCore/IO/BaseIO.h>
#include <FeCore/IO/Path.h>
#include <FeCore/Logging/Logger.h>

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
