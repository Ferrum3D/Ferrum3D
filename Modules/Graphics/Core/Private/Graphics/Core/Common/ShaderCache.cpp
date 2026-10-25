#include <Core/IO/Async.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Logging/Trace.h>
#include <Core/Math/Uuid.h>
#include <Core/Serialization/Document.h>
#include <Graphics/Core/Common/ShaderCache.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>

namespace FE::Graphics::Core
{
    namespace
    {
        //! Results kept per lookup key, e.g. to switch between branches without recompiling.
        constexpr uint32_t kMaxEntriesPerKey = 4;


        struct HexString final
        {
            char m_data[17];

            explicit HexString(const uint64_t value)
            {
                constexpr const char* kDigits = "0123456789abcdef";
                for (uint32_t index = 0; index < 16; ++index)
                    m_data[index] = kDigits[(value >> (60 - index * 4)) & 0xf];

                m_data[16] = '\0';
            }

            [[nodiscard]] festd::string_view Get() const
            {
                return festd::string_view{ m_data, 16 };
            }

            [[nodiscard]] festd::string_view GetShard() const
            {
                return festd::string_view{ m_data, 2 };
            }
        };


        IO::Path MakeCachePath(const IO::Path& rootDirectory, const festd::string_view category, const uint64_t key,
                               const festd::string_view extension)
        {
            const HexString hex{ key };

            IO::Path path = rootDirectory / category;
            path /= hex.GetShard();
            path /= Fmt::FixedFormat("{}{}", hex.Get(), extension);
            return path;
        }


        template<class TWriter>
        bool WriteFileAtomically(const IO::Path& path, TWriter&& writer)
        {
            if (IO::Directory::Create(IO::PathView{ path }.parent_directory()) != IO::ResultCode::kSuccess)
                return false;

            IO::Path temporaryPath = path;
            temporaryPath.AsBaseString() += Fmt::FixedFormat(".{}.tmp", Uuid::Random());

            {
                auto stream = IO::FileStream::Open(temporaryPath, IO::OpenMode::kCreate);
                if (!stream)
                    return false;

                const bool written = writer(*stream.value());
                stream.value()->Close();

                if (!written)
                {
                    IO::File::Delete(temporaryPath);
                    return false;
                }
            }

            // Another process may have published the same file meanwhile; both versions are equally valid.
            if (IO::File::Replace(temporaryPath, path) != IO::ResultCode::kSuccess)
            {
                IO::File::Delete(temporaryPath);
                return IO::File::Exists(path);
            }

            return true;
        }


        IO::ResultCode WaitForRead(IO::Async::Batch& batch)
        {
            const Rc completion = WaitGroup::Create();
            batch.SetCompletionWaitGroup(completion.Get());

            const Rc controller = IO::Async::Read(std::move(batch));
            completion->Wait();

            if (controller->GetStatus() == IO::Async::Status::kSucceeded)
                return IO::ResultCode::kSuccess;

            const IO::ResultCode result = controller->GetLastOperationResult();
            return result == IO::ResultCode::kSuccess ? IO::ResultCode::kUnknownError : result;
        }


        bool LoadIndex(const IO::Path& path, ShaderCacheIndex& index)
        {
            FE_PROFILER_ZONE();

            festd::pmr::vector<std::byte> bytes;
            IO::Async::Batch batch;
            batch.SetSource(IO::PathView{ path });
            batch.ReadAppendToEnd(bytes);
            if (WaitForRead(batch) != IO::ResultCode::kSuccess)
                return false;

            IO::ReadOnlyMemoryStream stream{ festd::span<const std::byte>{ bytes.data(), bytes.size() } };
            return Serialization::LoadDocument(stream, index) == Serialization::ResultCode::kSuccess;
        }


        bool AreDependenciesCurrent(const ShaderCacheEntry& entry, ShaderSourceCache& sources)
        {
            if (entry.m_dependencies.empty())
                return false;

            // Reading the recorded dependencies in parallel also warms the source cache for a recompile after a miss.
            festd::inline_vector<Env::Name, 32> paths;
            for (const ShaderCacheDependency& dependency : entry.m_dependencies)
                paths.push_back(dependency.m_path);

            sources.Prefetch(festd::span{ paths.data(), paths.size() });

            for (const ShaderCacheDependency& dependency : entry.m_dependencies)
            {
                const festd::expected source = sources.GetSource(dependency.m_path);
                if (!source)
                    return false;

                if (source.value()->GetContentHash() != dependency.m_contentHash)
                    return false;
            }

            return true;
        }


        bool HaveSameDependencies(const ShaderCacheEntry& lhs, const ShaderCacheEntry& rhs)
        {
            if (lhs.m_dependencies.size() != rhs.m_dependencies.size())
                return false;

            for (uint32_t index = 0; index < lhs.m_dependencies.size(); ++index)
            {
                const ShaderCacheDependency& left = lhs.m_dependencies[index];
                const ShaderCacheDependency& right = rhs.m_dependencies[index];
                if (left.m_path != right.m_path || left.m_contentHash != right.m_contentHash)
                    return false;
            }

            return true;
        }
    } // namespace


    ShaderCache::ShaderCache(IO::Path rootDirectory)
        : m_rootDirectory(std::move(rootDirectory))
    {
    }


    bool ShaderCache::TryLoad(const uint64_t lookupKey, ShaderSourceCache& sources, std::pmr::memory_resource* allocator,
                              ShaderCompilerResult& result) const
    {
        FE_PROFILER_ZONE();

        ShaderCacheIndex index;
        if (!LoadIndex(MakeCachePath(m_rootDirectory, "Index", lookupKey, ".index"), index))
            return false;

        for (const ShaderCacheEntry& entry : index.m_entries)
        {
            if (!AreDependenciesCurrent(entry, sources))
                continue;

            const IO::Path byteCodePath = MakeCachePath(m_rootDirectory, "ByteCode", entry.m_byteCodeHash, ".spv");

            const uint32_t alignedSize = AlignUp<sizeof(uint32_t)>(entry.m_byteCodeSize);
            ByteBuffer byteCode{ alignedSize, allocator };
            memset(byteCode.data() + entry.m_byteCodeSize, 0, alignedSize - entry.m_byteCodeSize);

            // A shorter file fails the read; a longer one fails the hash check below.
            IO::Async::Batch batch;
            batch.SetSource(IO::PathView{ byteCodePath }, entry.m_byteCodeSize);
            batch.Read(byteCode.data(), entry.m_byteCodeSize);
            if (WaitForRead(batch) != IO::ResultCode::kSuccess)
                continue;

            if (DefaultHash(byteCode.data(), entry.m_byteCodeSize) != entry.m_byteCodeHash)
            {
                Logger::LogWarning("Ignoring corrupted shader cache file '{}'", byteCodePath);
                continue;
            }

            result.m_byteCode = std::move(byteCode);
            result.m_byteCodeSize = entry.m_byteCodeSize;
            result.m_hash = entry.m_byteCodeHash;
            result.m_hashValid = true;
            result.m_codeValid = true;
            for (const ShaderCacheDependency& dependency : entry.m_dependencies)
                result.m_dependencies.push_back(dependency.m_path);

            return true;
        }

        return false;
    }


    void ShaderCache::Store(const uint64_t lookupKey, const festd::span<const ShaderCacheDependency> dependencies,
                            const festd::span<const std::byte> byteCode) const
    {
        FE_PROFILER_ZONE();

        ShaderCacheEntry entry;
        entry.m_dependencies.assign(dependencies.begin(), dependencies.end());
        entry.m_byteCodeHash = DefaultHash(byteCode.data(), byteCode.size());
        entry.m_byteCodeSize = byteCode.size();

        const IO::Path byteCodePath = MakeCachePath(m_rootDirectory, "ByteCode", entry.m_byteCodeHash, ".spv");
        if (!IO::File::Exists(byteCodePath) && !WriteShaderCacheFile(byteCodePath, byteCode))
        {
            Logger::LogWarning("Failed to write shader cache file '{}'", byteCodePath);
            return;
        }

        const IO::Path indexPath = MakeCachePath(m_rootDirectory, "Index", lookupKey, ".index");

        // A missing or unreadable index is replaced with a new one.
        ShaderCacheIndex index;
        LoadIndex(indexPath, index);

        index.m_entries.erase(eastl::remove_if(index.m_entries.begin(),
                                               index.m_entries.end(),
                                               [&entry](const ShaderCacheEntry& existing) {
                                                   return HaveSameDependencies(existing, entry);
                                               }),
                              index.m_entries.end());

        index.m_entries.insert(index.m_entries.begin(), std::move(entry));
        if (index.m_entries.size() > kMaxEntriesPerKey)
            index.m_entries.resize(kMaxEntriesPerKey);

        const bool written = WriteFileAtomically(indexPath, [&index](IO::FileStream& stream) {
            Serialization::PackedBinaryFormat format;
            Serialization::SerializationContext context(&stream, format);
            return context.Store(index) == Serialization::ResultCode::kSuccess;
        });

        if (!written)
            Logger::LogWarning("Failed to write shader cache file '{}'", indexPath);
    }


    IO::Path GetShaderCacheDirectory()
    {
        return IO::Directory::GetExecutableDirectory() / "ShaderCache";
    }


    bool WriteShaderCacheFile(const IO::Path& path, const festd::span<const std::byte> data)
    {
        return WriteFileAtomically(path, [data](IO::FileStream& stream) {
            return stream.WriteFromBuffer(data.data(), data.size()) == data.size();
        });
    }
} // namespace FE::Graphics::Core
