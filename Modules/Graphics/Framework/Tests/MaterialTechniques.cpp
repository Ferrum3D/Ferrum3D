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
