#include <Framework/Module.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Module.h>

#include "App.h"


using namespace FE;


int main(const int32_t argc, const char** argv)
{
    Framework::Module::Init();
    Graphics::Core::Module::Init();
    Graphics::Module::Init();

    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = "AssetBuilder";
    Env::Init(applicationInfo);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);

    auto* application = Memory::New<AssetBuilder::App>(allocator, argc, argv);
    application->InitializeCore();

    IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();

    int32_t exitCode = 0;
    FunctorJob mainJob([application, jobSystem, &exitCode] {
        application->InitializeApp();
        exitCode = application->Run();
        jobSystem->Stop();
    });

    mainJob.Schedule(jobSystem, FiberAffinityMask::kMainThread);
    jobSystem->Start();

    Memory::Delete(allocator, application);
    Env::Module::ShutdownModules();
    return exitCode;
}
