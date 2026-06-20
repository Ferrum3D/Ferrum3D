#include <FerrumCli/App.h>

#include <Core/IO/BaseIO.h>
#include <festd/vector.h>

using namespace FE;

int main(const int32_t argc, const char** argv)
{
    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = "FerrumCli";
    Env::Init(applicationInfo);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);

    festd::pmr::vector<festd::string_view> commandLine{ allocator };
    commandLine.reserve(argc);

    for (int32_t index = 1; index < argc; ++index)
        commandLine.emplace_back(argv[index]);

    int32_t exitCode;
    {
        FerrumCli::App application{ commandLine };
        exitCode = application.Run();
        IO::Flush(IO::StandardDescriptor::kStdout);
        IO::Flush(IO::StandardDescriptor::kStderr);
    }

    return exitCode;
}
