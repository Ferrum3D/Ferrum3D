#include <FeCore/DI/Builder.h>
#include <Graphics/Assets/ModelAssetManager.h>
#include <Graphics/Assets/TextureAssetManager.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Module.h>

namespace FE::Graphics
{
    namespace
    {
        void ModuleDependencyHandler()
        {
            Core::Module::Init();
        }
    } // namespace


    void Module::RegisterServices(const DI::ServiceRegistryBuilder& builder)
    {
        FE_PROFILER_ZONE();

        builder.Bind<ITextureAssetManager>().To<TextureAssetManager>().InSingletonScope();
        builder.Bind<IModelAssetManager>().To<ModelAssetManager>().InSingletonScope();
    }

    FE_IMPLEMENT_MODULE(Module, ModuleDependencyHandler);
} // namespace FE::Graphics
