#pragma once
#include <Core/Memory/RefCount.h>
#include <Core/RTTI/RTTI.h>
#include <festd/vector.h>

namespace FE::Graphics::Core
{
    //! @brief A source file version that a precompiled shader was compiled from.
    struct PrecompiledShaderDependency final
    {
        FE_RTTI_Reflect("24BD8283-6A32-4B36-8152-4B51D8D93387");
        FE_RTTI_Serialize();

        Env::Name m_path;
        uint64_t m_contentHash = 0;
    };


    //! @brief One compiled shader, identified by ComputeShaderRequestKey.
    struct PrecompiledShader final
    {
        FE_RTTI_Reflect("043431C9-5954-497E-BA3C-6C3E5845279A");
        FE_RTTI_Serialize();

        uint64_t m_requestKey = 0;
        festd::vector<PrecompiledShaderDependency> m_dependencies;

        //! SPIR-V words; the last word is zero-padded when m_byteCodeSize is not a multiple of four.
        festd::vector<uint32_t> m_byteCode;
        uint32_t m_byteCodeSize = 0;
    };


    //! @brief Shaders compiled ahead of time that the shader compiler consults before its persistent cache.
    //!
    //! A precompiled shader is used only if its dependencies match the current sources, like an entry of the persistent cache.
    //! When every requested shader is found, the compiler itself is never loaded.
    struct PrecompiledShaderSet final : public Memory::RefCountedObjectBase
    {
        explicit PrecompiledShaderSet(festd::vector<PrecompiledShader>&& shaders);

        //! @brief Append the shaders compiled for a request key.
        void Find(uint64_t requestKey, festd::inline_vector<const PrecompiledShader*, 4>& result) const;

        [[nodiscard]] uint32_t GetShaderCount() const
        {
            return m_shaders.size();
        }

    private:
        void DoRelease() override;

        festd::vector<PrecompiledShader> m_shaders;
    };


    //! @brief Make precompiled shaders visible to every shader compiler, replacing the previous set. Pass null to remove them.
    void SetPrecompiledShaders(PrecompiledShaderSet* shaders);

    //! @brief Get the precompiled shaders that are currently visible, if any.
    [[nodiscard]] Rc<PrecompiledShaderSet> GetPrecompiledShaders();
} // namespace FE::Graphics::Core
