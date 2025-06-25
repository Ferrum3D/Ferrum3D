#include "App.h"
#include "ModelProcessor.h"
#include "TextureProcessor.h"

#include <FeCore/IO/IStreamFactory.h>

namespace FE::AssetBuilder
{
    App::App(const int32_t argc, const char** argv)
        : Application(argc, argv)
    {
    }


    void App::InitializeApp()
    {
        FE_PROFILER_ZONE();

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();

        m_logger = serviceProvider->ResolveRequired<Logger>();
        m_logSink = festd::make_unique<Framework::StdoutLogSink>(m_logger.Get());
    }


    Rc<WaitGroup> App::ScheduleUpdate()
    {
        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        IO::IStreamFactory* streamFactory = serviceProvider->ResolveRequired<IO::IStreamFactory>();

        const auto args = CommandLine::Get();
        for (const festd::string_view arg : args)
        {
            const IO::PathView pathView{ arg };
            const IO::Path fullInputPath = IO::GetAbsolutePath(arg);

            IO::Path outputPath = pathView.parent_directory();
            outputPath /= pathView.stem();

            if (pathView.extension() == ".glb")
            {
                ModelProcessSettings settings;
                settings.m_logger = m_logger.Get();
                settings.m_streamFactory = streamFactory;
                settings.m_inputFile = fullInputPath;
                settings.m_outputFile = outputPath;
                settings.m_outputFile.append(".fmd");
                ProcessModel(settings);
            }
            else
            {
                TextureProcessSettings settings;
                settings.m_logger = m_logger.Get();
                settings.m_streamFactory = streamFactory;
                settings.m_inputFile = fullInputPath;
                settings.m_outputFile = outputPath;
                settings.m_outputFile.append(".ftx");
                ProcessTexture(settings);
            }
        }

        return nullptr;
    }
} // namespace FE::AssetBuilder
