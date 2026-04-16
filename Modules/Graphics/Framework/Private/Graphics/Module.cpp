#include <FeCore/DI/Builder.h>
#include <Graphics/Assets/ModelAssetManager.h>
#include <Graphics/Assets/TextureAssetManager.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Module.h>
#include <Graphics/RendererImpl.h>

namespace FE::Graphics
{
    void Module::RegisterServices(const DI::ServiceRegistryBuilder& builder)
    {
        FE_PROFILER_ZONE();

        builder.Bind<Renderer>().To<RendererImpl>().InSingletonScope();
        builder.Bind<ITextureAssetManager>().To<TextureAssetManager>().InSingletonScope();
        builder.Bind<IModelAssetManager>().To<ModelAssetManager>().InSingletonScope();
    }


    static void ModuleDependencyHandler()
    {
        Core::Module::Init();
    }

    FE_IMPLEMENT_MODULE(Module, ModuleDependencyHandler);
} // namespace FE::Graphics
