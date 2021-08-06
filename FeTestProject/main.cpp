#include <FeCore/Console/FeLog.h>
#include <FeCore/Math/Vector4.h>
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Instance/IInstance.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

struct Vertex
{
    float X, Y, Z;
};

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

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(800, 600, "Test project", nullptr, nullptr);

        FE::GPU::SwapChainDesc swapChainDesc{};
        swapChainDesc.ImageCount         = 3;
        swapChainDesc.ImageWidth         = 800;
        swapChainDesc.ImageHeight        = 600;
        swapChainDesc.NativeWindowHandle = glfwGetWin32Window(window);
        swapChainDesc.Queue              = queue.GetRaw();
        swapChainDesc.VerticalSync       = true;
        auto swapChain                   = device->CreateSwapChain(swapChainDesc);

        {
            std::vector<Vertex> vertexData = { { -0.5, -0.5, 0.0 }, { 0.0, 0.5, 0.0 }, { 0.5, -0.5, 0.0 } };
            auto vertexSize                = vertexData.size() * sizeof(Vertex);
            auto vertexBuffer              = device->CreateBuffer(FE::GPU::BindFlags::VertexBuffer, vertexSize);
            vertexBuffer->AllocateMemory(FE::GPU::MemoryType::HostVisible);
            void* map = vertexBuffer->Map(0);
            memcpy(map, vertexData.data(), vertexSize);
            vertexBuffer->Unmap();
        }
        {
            std::vector<uint32_t> indexData = { 0, 1, 2 };
            auto indexSize                  = indexData.size() * sizeof(uint32_t);
            auto indexBuffer                = device->CreateBuffer(FE::GPU::BindFlags::IndexBuffer, indexSize);
            indexBuffer->AllocateMemory(FE::GPU::MemoryType::HostVisible);
            void* map = indexBuffer->Map(0);
            memcpy(map, indexData.data(), indexSize);
            indexBuffer->Unmap();
        }
        {
            FE::float4 constantData = { 1, 0, 0, 1 };
            auto constantBuffer     = device->CreateBuffer(FE::GPU::BindFlags::ConstantBuffer, sizeof(FE::float4));
            constantBuffer->AllocateMemory(FE::GPU::MemoryType::HostVisible);
            void* map = constantBuffer->Map(0);
            memcpy(map, constantData.Data(), sizeof(FE::float4));
            constantBuffer->Unmap();
        }

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
}
