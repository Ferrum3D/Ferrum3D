#include <FeCore/Console/FeLog.h>
#include <FeCore/Math/Vector4.h>
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Instance/IInstance.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "PixelShader.h"
#include "VertexShader.h"

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
        FE_LOG_MESSAGE(
            "Running {} version {}.{}.{}", FE::StringSlice(FE::FerrumEngineName), FE::FerrumVersion.Major,
            FE::FerrumVersion.Minor, FE::FerrumVersion.Patch);

        FE::GPU::InstanceDesc desc{};
        auto instance = FE::GPU::CreateGraphicsAPIInstance(desc, FE::GPU::GraphicsAPI::Vulkan);
        auto adapter  = instance->GetAdapters()[0];
        auto device   = adapter->CreateDevice();
        auto fence    = device->CreateFence(FE::GPU::FenceState::Reset);
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

        FE::GPU::ShaderModuleDesc pixelDesc;
        pixelDesc.EntryPoint   = "main";
        pixelDesc.ByteCode     = PixelShader;
        pixelDesc.ByteCodeSize = sizeof(PixelShader);
        pixelDesc.Stage        = FE::GPU::ShaderStage::Pixel;
        auto pixelShader       = device->CreateShaderModule(pixelDesc);

        FE::GPU::ShaderModuleDesc vertexDesc;
        vertexDesc.EntryPoint   = "main";
        vertexDesc.ByteCode     = VertexShader;
        vertexDesc.ByteCodeSize = sizeof(VertexShader);
        vertexDesc.Stage        = FE::GPU::ShaderStage::Vertex;
        auto vertexShader       = device->CreateShaderModule(vertexDesc);

        FE::GPU::RenderPassDesc renderPassDesc{};

        FE::GPU::AttachmentDesc attachmentDesc{};
        attachmentDesc.Format       = swapChain->GetDesc().Format;
        attachmentDesc.StoreOp      = FE::GPU::AttachmentStoreOp::Store;
        attachmentDesc.LoadOp       = FE::GPU::AttachmentLoadOp::Clear;
        attachmentDesc.InitialState = FE::GPU::ResourceState::Undefined;
        attachmentDesc.FinalState   = FE::GPU::ResourceState::Present;

        renderPassDesc.Attachments = { attachmentDesc };

        FE::GPU::SubpassDesc subpassDesc{};
        subpassDesc.RenderTargetAttachments = { FE::GPU::SubpassAttachment(FE::GPU::ResourceState::RenderTarget, 0) };
        renderPassDesc.Subpasses            = { subpassDesc };

        FE::GPU::SubpassDependency dependency{};
        renderPassDesc.SubpassDependencies = { dependency };

        auto renderPass = device->CreateRenderPass(renderPassDesc);

        FE::GPU::DescriptorHeapDesc descriptorHeapDesc{};
        descriptorHeapDesc.MaxSets = 1;
        descriptorHeapDesc.Sizes   = { FE::GPU::DescriptorSize(1, FE::GPU::ShaderResourceType::ConstantBuffer) };
        auto descriptorHeap        = device->CreateDescriptorHeap(descriptorHeapDesc);

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
}
