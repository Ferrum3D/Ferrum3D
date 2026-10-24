#pragma once
#include <Core/IO/Path.h>
#include <Core/RTTI/RTTI.h>
#include <Graphics/Core/ShaderCompiler.h>
#include <festd/vector.h>

namespace FE::Graphics::Core
{
    struct ShaderSourceCache;

    //! @brief A source file version that a cached shader was compiled from.
    struct ShaderCacheDependency final
    {
        FE_RTTI_Reflect("F14DA666-767D-4A71-8D83-B637F577079D");
        FE_RTTI_Serialize();

        Env::Name m_path;
        uint64_t m_contentHash = 0;
    };


    //! @brief One compiled result and every source file version it depends on.
    struct ShaderCacheEntry final
    {
        FE_RTTI_Reflect("DB75FC0F-7E1B-4E5A-8082-95D68CFDB5EE");
        FE_RTTI_Serialize();

        festd::vector<ShaderCacheDependency> m_dependencies;
        uint64_t m_byteCodeHash = 0;
        uint32_t m_byteCodeSize = 0;
    };


    //! @brief Recent results of one compilation request, most recent first.
    struct ShaderCacheIndex final
    {
        FE_RTTI_Reflect("F8489BB4-9741-4E8D-AB16-302572B43DF0");
        FE_RTTI_Serialize();

        festd::vector<ShaderCacheEntry> m_entries;
    };


    //! @brief Persistent compiled shader cache shared by all executables in one directory.
    //!
    //! A compilation request is identified by a lookup key that hashes the compiler version, the complete argument list and
    //! the root shader path. The included files are only known after compiling, so the index of each key records the
    //! dependencies of its last few results. A result is reused when every dependency still has its recorded content hash.
    //! Bytecode is stored in separate files addressed by its own hash. All writes go through a temporary file and an atomic
    //! replace, so several processes can share the cache.
    struct ShaderCache final
    {
        explicit ShaderCache(IO::Path rootDirectory);

        //! @brief Load a result whose dependencies match the current sources.
        bool TryLoad(uint64_t lookupKey, ShaderSourceCache& sources, std::pmr::memory_resource* allocator,
                     ShaderCompilerResult& result) const;

        //! @brief Record a compiled result; failures are logged and otherwise ignored.
        void Store(uint64_t lookupKey, festd::span<const ShaderCacheDependency> dependencies,
                   festd::span<const std::byte> byteCode) const;

    private:
        IO::Path m_rootDirectory;
    };


    //! @brief Directory for persistent shader and pipeline caches, located next to the executable.
    IO::Path GetShaderCacheDirectory();


    //! @brief Write a cache file through a temporary file and an atomic replace.
    bool WriteShaderCacheFile(const IO::Path& path, festd::span<const std::byte> data);
} // namespace FE::Graphics::Core
