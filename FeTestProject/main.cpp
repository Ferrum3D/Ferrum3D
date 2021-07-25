#include <FeCore/Console/FeLog.h>
#include <FeCore/Jobs/FeJobSystem.h>
#include <FeCore/Math/FeVector3.h>
#include <FeCore/Time/DateTime.h>
#include <FeCore/Utils/Result.h>
#include <FeRenderer/FeRenderer.h>
#include <array>
#include <iostream>

const std::string g_TestPixelSource = R"(
Texture2D    g_Texture;
SamplerState g_Texture_sampler;

struct PSInput
{
	float4 Pos : SV_POSITION;
	float2 UV : TEX_COORD;
};

struct PSOutput
{
	float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
	PSOut.Color = g_Texture.Sample(g_Texture_sampler, PSIn.UV);
}
)";

int main()
{
    FE::InitLogger();
    FE::LogMsg("\n{}\n{}\n{}", FE::TypeName<int>(), FE::TypeName<FE::IFeGraphicsDevice>(), FE::TypeName<FE::FeGraphicsDeviceDesc>(), FE::TypeName<std::string>());
    FE::LogMsg("\n{}\n{}\n{}", FE::TypeHash<int>(), FE::TypeHash<FE::IFeGraphicsDevice>(), FE::TypeHash<FE::FeGraphicsDeviceDesc>(), FE::TypeHash<std::string>());

    auto window = FE::CreateMainWindow(800, 600);
    window->Init();

    auto device = FE::CreateGraphicsDevice(window.get(), FE::FeGraphicsDeviceDesc{});
    FE::FeShaderLoadDesc desc{};
    desc.Name = "test pixel";
    desc.Type = FE::FeShaderType::Pixel;

    auto ps = device->CreateShader(desc, g_TestPixelSource);

    while (!window->ShouldClose())
    {
        window->PollEvents();
    }

    window->Close();
}
