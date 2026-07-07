#include <FerrumCli/App.h>

#include <Core/IO/BaseIO.h>
#include <Core/Jobs/Jobs.h>
#include <festd/vector.h>

using namespace FE;

int main(const int32_t argc, const char** argv)
{
    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = "FerrumCli";
    Env::Init(applicationInfo, argc, argv);

    int32_t exitCode = 0;
    Jobs::DispatchMainThread([&exitCode] {
        FerrumCli::App application;
        exitCode = application.Run();
        IO::Flush(IO::StandardDescriptor::kStdout);
        IO::Flush(IO::StandardDescriptor::kStderr);
        Jobs::StopJobSystem();
    });
    Jobs::StartJobSystem();

    return exitCode;
}
