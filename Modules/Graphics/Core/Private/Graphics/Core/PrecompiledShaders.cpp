#include <Core/Threading/SpinLock.h>
#include <Graphics/Core/PrecompiledShaders.h>

namespace FE::Graphics::Core
{
    namespace
    {
        struct PrecompiledShaderRegistry final
        {
            Threading::SpinLock m_lock;
            Rc<PrecompiledShaderSet> m_shaders;
        };


        PrecompiledShaderRegistry& GetRegistry()
        {
            static PrecompiledShaderRegistry registry;
            return registry;
        }
    } // namespace


    PrecompiledShaderSet::PrecompiledShaderSet(festd::vector<PrecompiledShader>&& shaders)
        : m_shaders(std::move(shaders))
    {
    }


    void PrecompiledShaderSet::Find(const uint64_t requestKey, festd::inline_vector<const PrecompiledShader*, 4>& result) const
    {
        for (const PrecompiledShader& shader : m_shaders)
        {
            if (shader.m_requestKey == requestKey)
                result.push_back(&shader);
        }
    }


    void PrecompiledShaderSet::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void SetPrecompiledShaders(PrecompiledShaderSet* shaders)
    {
        PrecompiledShaderRegistry& registry = GetRegistry();
        std::lock_guard lock{ registry.m_lock };
        registry.m_shaders = shaders;
    }


    Rc<PrecompiledShaderSet> GetPrecompiledShaders()
    {
        PrecompiledShaderRegistry& registry = GetRegistry();
        std::lock_guard lock{ registry.m_lock };
        return registry.m_shaders;
    }
} // namespace FE::Graphics::Core
