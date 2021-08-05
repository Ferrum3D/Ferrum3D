#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>

#include <FeGPU/Instance/IInstance.h>

int main()
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    {
        auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();
        FE::GPU::InstanceDesc desc{};
        auto instance = FE::GPU::CreateGraphicsAPIInstance(desc, FE::GPU::GraphicsAPI::Vulkan);
        auto adapter  = instance->GetAdapters()[0];
        auto device   = adapter->CreateDevice();
        auto fence    = device->CreateFence(0);
        auto queue    = device->GetCommandQueue(FE::GPU::CommandQueueClass::Graphics);
        auto buffer   = device->CreateCommandBuffer(FE::GPU::CommandQueueClass::Graphics);
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
}
