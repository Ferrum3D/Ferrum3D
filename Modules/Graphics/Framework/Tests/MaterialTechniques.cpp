#include <AssetBuilder/MaterialProcessor.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    bool Validate(const char* body)
    {
        const festd::string source =
            Fmt::Format("local Material = require('@engine/material')\nlocal m = Material.new('Test')\n{}\nreturn m", body);
        return AssetBuilder::ValidateMaterialSource(
            IO::Path("Stage1.mat.luau"),
            festd::span(reinterpret_cast<const std::byte*>(source.data()), source.size()));
    }
} // namespace


TEST(MaterialTechniques, ComputeRolesAndParameterFreePalette)
{
    EXPECT_TRUE(Validate(R"(
m:specInt('Palette', {0, 1}, 0)
m:technique('SurfaceEvaluate', function(t, ctx)
    t.computeShader = 'Evaluate.cs'
    t.shaderDefines['PALETTE'] = tostring(ctx:getInt('Palette'))
    return true
end)
m:technique('DeferredShade', function(t, ctx)
    t.computeShader = 'SimpleLit.cs'
    return true
end)
)"));
}


TEST(MaterialTechniques, RejectsGraphicsFieldsInComputeTechnique)
{
    EXPECT_FALSE(Validate(R"(
m:technique('SurfaceEvaluate', function(t, ctx)
    t.computeShader = 'Evaluate.cs'
    t.renderTargetFormats = {'R32_UINT'}
    return true
end)
)"));
}


TEST(MaterialTechniques, RejectsWrongRoleKindAndMissingShader)
{
    EXPECT_FALSE(Validate(R"(
m:technique('SurfaceEvaluate', function(t, ctx)
    t.kind = 'Graphics'
    t.meshShader = 'Mesh.ms'
    return true
end)
)"));
    EXPECT_FALSE(Validate(R"(
m:technique('DeferredShade', function(t, ctx)
    return true
end)
)"));
}


TEST(MaterialTechniques, IndependentTransparencyTargets)
{
    EXPECT_TRUE(Validate(R"(
local Pipeline = require('@engine/pipeline')
m:technique('ForwardTransparency', function(t, ctx)
    t.meshShader = 'Transparent.ms'
    t.pixelShader = 'Transparent.ps'
    t.renderTargetFormats = { 'R16G16B16A16_SFLOAT', 'R16_SFLOAT' }
    t.targetBlends = { Pipeline.BlendState.disabled(), Pipeline.BlendState.disabled() }
    return true
end)
)"));
    EXPECT_FALSE(Validate(R"(
local Pipeline = require('@engine/pipeline')
m:technique('ForwardTransparency', function(t, ctx)
    t.meshShader = 'Transparent.ms'
    t.pixelShader = 'Transparent.ps'
    t.renderTargetFormats = { 'R16G16B16A16_SFLOAT', 'R16_SFLOAT' }
    t.targetBlends = { Pipeline.BlendState.disabled() }
    return true
end)
)"));
}
