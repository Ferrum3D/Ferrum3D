#pragma once
#include <Core/IO/BaseIO.h>
#include <Core/Memory/RefCount.h>
#include <Core/Threading/SharedSpinLock.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::Graphics::Core
{
    //! @brief Immutable contents of one shader source file, identified by its virtual path.
    struct ShaderSourceFile final : public Memory::RefCountedObjectBase
    {
        //! @brief Take ownership of the file contents; a null terminator is appended.
        ShaderSourceFile(Env::Name path, festd::pmr::vector<std::byte>&& source);

        [[nodiscard]] Env::Name GetPath() const
        {
            return m_path;
        }

        //! @brief Get the file contents. The returned view is null-terminated.
        [[nodiscard]] festd::string_view GetSource() const
        {
            return festd::string_view{ reinterpret_cast<const char*>(m_source.data()), m_source.size() - 1 };
        }

        [[nodiscard]] uint64_t GetContentHash() const
        {
            return m_contentHash;
        }

    private:
        void DoRelease() override;

        Env::Name m_path;
        festd::pmr::vector<std::byte> m_source;
        uint64_t m_contentHash = 0;
    };


    enum class ShaderSourceReadMode : uint32_t
    {
        //! Read through the asynchronous I/O scheduler and suspend the calling fiber until the read completes.
        kAsync,

        //! Read on the calling thread without switching fibers. Required inside shader compiler callbacks: DXC keeps
        //! per-thread state for the duration of a compilation, so its thread must not run another fiber meanwhile.
        kBlocking,
    };


    //! @brief Lazily loads shader sources from the directories registered with AddShaderSourceDirectory.
    //!
    //! A file is read on its first request and stays unchanged for the lifetime of the cache, so all compilations see the
    //! same version of a shared header. Missing files are remembered too, because the compiler probes several candidate
    //! paths for every include.
    struct ShaderSourceCache final : public Memory::RefCountedObjectBase
    {
        FE_RTTI("FE08F0A8-40B4-4C17-B152-8220DC1BF5F6");

        festd::expected<Rc<ShaderSourceFile>, IO::ResultCode> GetSource(Env::Name path,
                                                                        ShaderSourceReadMode mode = ShaderSourceReadMode::kAsync);

        //! @brief Read every path that is not cached yet with parallel asynchronous reads.
        void Prefetch(festd::span<const Env::Name> paths);

    private:
        festd::optional<Rc<ShaderSourceFile>> TryGetCached(Env::Name path);
        static festd::expected<Rc<ShaderSourceFile>, IO::ResultCode> MakeResult(const Rc<ShaderSourceFile>& file);

        void DoRelease() override;

        Threading::SharedSpinLock m_lock;
        festd::segmented_unordered_dense_map<Env::Name, Rc<ShaderSourceFile>> m_filesMap;
    };
} // namespace FE::Graphics::Core
