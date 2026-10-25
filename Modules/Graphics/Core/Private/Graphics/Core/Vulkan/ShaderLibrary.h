#pragma once
#include <Core/Containers/SegmentedVector.h>
#include <Core/IO/BaseIO.h>
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/ShaderCompiler.h>
#include <Graphics/Core/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderReflection;

    //! @brief One compiled version of a shader.
    //!
    //! A recompilation publishes a new version and leaves the previous one alive while anything still references it, e.g. a
    //! pipeline that is being created from it.
    struct ShaderVersion final : public Memory::RefCountedObjectBase
    {
        ShaderVersion(VkDevice device, VkShaderModule shaderModule, ShaderReflection* reflection);
        ~ShaderVersion() override;

        [[nodiscard]] VkShaderModule GetModule() const
        {
            return m_shaderModule;
        }

        [[nodiscard]] ShaderReflection* GetReflection() const
        {
            return m_reflection.Get();
        }

        [[nodiscard]] const char* GetEntryPoint() const
        {
            return "main";
        }

    private:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        VkDevice m_device = VK_NULL_HANDLE;
        VkShaderModule m_shaderModule = VK_NULL_HANDLE;
        Rc<ShaderReflection> m_reflection;
    };


    struct ShaderLibrary final : public Core::ShaderLibrary
    {
        FE_RTTI("E2254CBD-679C-4310-87CF-FA8DA780BDA1");

        ShaderLibrary(Core::Device* device, Core::ShaderCompiler* shaderCompiler);
        ~ShaderLibrary() override;

        Core::ShaderHandle GetShader(Env::Name name, Env::Name defines) override;
        Rc<Core::ShaderReflection> GetCompiledReflection(Env::Name name, Env::Name defines) override;

        //! @brief Find a shader without starting a compilation. Returns an invalid handle if it was never requested.
        [[nodiscard]] Core::ShaderHandle FindShader(Env::Name name, Env::Name defines);

        [[nodiscard]] WaitGroup* GetCompletionWaitGroup(Core::ShaderHandle shaderHandle) const;

        //! @brief Get the current version of a compiled shader, or null if it failed to compile.
        [[nodiscard]] Rc<ShaderVersion> GetVersion(Core::ShaderHandle shaderHandle);

        //! @brief Append every compiled shader whose sources include one of the paths.
        void FindDependentShaders(festd::span<const Env::Name> paths, festd::vector<Core::ShaderHandle>& shaders);

        //! @brief Compile the current sources of a shader and publish the result.
        //!
        //! @return True if a new version was published. On failure the previous version stays current.
        bool Recompile(Core::ShaderHandle shaderHandle);

    private:
        struct ShaderInfo final
        {
            Env::Name m_name;
            Env::Name m_defines;
            Rc<WaitGroup> m_completionWaitGroup;

            // Guarded by ShaderLibrary::m_lock once the initial compilation has completed.
            Rc<ShaderVersion> m_version;
            festd::inline_vector<Env::Name, 16> m_dependencies;
        };

        void CompileShader(uint32_t shaderIndex);
        Rc<ShaderVersion> CompileVersion(const ShaderInfo& shaderInfo, festd::inline_vector<Env::Name, 16>& dependencies);

        void DestroyObject() override
        {
            Memory::DefaultDelete(this);
        }

        Threading::SpinLock m_lock;
        Memory::SpinLockedPoolAllocator m_shaderPool;
        festd::unordered_dense_map<uint64_t, uint32_t> m_shadersMap;
        SegmentedVector<ShaderInfo*> m_shaders;
        Core::ShaderCompiler* m_shaderCompiler = nullptr;
    };
} // namespace FE::Graphics::Vulkan
