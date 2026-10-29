#include <AssetBuilder/BuildState.h>
#include <Core/IO/FileStream.h>
#include <Core/IO/MemoryStream.h>
#include <Core/Math/Uuid.h>
#include <Core/Serialization/Document.h>
#include <Core/Strings/Utils.h>

namespace FE::AssetBuilder
{
    namespace
    {
        IO::Path GetBuildStatePath(const IO::Path& outputDirectory, const IO::AssetID assetId)
        {
            const auto idString = Str::ToLower<festd::fixed_string>(Fmt::FixedFormat("{}", assetId));

            IO::Path path = GetBuildDirectory(outputDirectory) / "state";
            path /= festd::string_view{ idString.data(), 2 };
            path /= Fmt::FixedFormat("{}.json", idString);
            return path;
        }
    } // namespace


    IO::Path GetBuildDirectory(const IO::Path& outputDirectory)
    {
        return outputDirectory / ".ferrum-build";
    }


    bool LoadBuildState(const IO::Path& outputDirectory, const IO::AssetID assetId, ProductBuildState& state)
    {
        auto stream = IO::FileStream::Open(GetBuildStatePath(outputDirectory, assetId), IO::OpenMode::kReadOnly);
        if (!stream)
            return false;

        if (Serialization::LoadDocument(*stream.value(), state) == Serialization::ResultCode::kSuccess)
            return true;

        // A partially read state must not make a product look up to date.
        state = {};
        return false;
    }


    bool StoreBuildState(const IO::Path& outputDirectory, const IO::AssetID assetId, const ProductBuildState& state)
    {
        festd::pmr::vector<std::byte> bytes;
        IO::WriteOnlyVectorStream stream{ bytes };
        Serialization::JsonFormat format;
        Serialization::SerializationContext context(&stream, format);
        if (context.Store(state) != Serialization::ResultCode::kSuccess)
            return false;

        return WriteFileAtomically(GetBuildStatePath(outputDirectory, assetId), bytes);
    }


    void RemoveBuildState(const IO::Path& outputDirectory, const IO::AssetID assetId)
    {
        const IO::Path path = GetBuildStatePath(outputDirectory, assetId);
        if (IO::File::Exists(path))
            IO::File::Delete(path);
    }


    void RemoveDirectoryTree(const IO::Path& directory)
    {
        festd::vector<IO::Path> files;
        festd::vector<IO::Path> directories;
        IO::Directory::TraverseRecursively(directory, [&files, &directories](const IO::DirectoryEntry& entry) {
            if (Bit::AnySet(entry.m_attributes, IO::FileAttributeFlags::kDirectory))
                directories.push_back(IO::Path{ entry.m_path });
            else
                files.push_back(IO::Path{ entry.m_path });

            return true;
        });

        for (const IO::Path& file : files)
            IO::File::Delete(file);

        // Traversal visits parents before their children, so deleting in reverse order removes children first.
        for (uint32_t index = directories.size(); index > 0; --index)
            IO::Directory::Delete(directories[index - 1]);

        IO::Directory::Delete(directory);
    }


    bool WriteFileAtomically(const IO::Path& path, const festd::span<const std::byte> bytes)
    {
        if (IO::Directory::Create(IO::PathView{ path }.parent_directory()) != IO::ResultCode::kSuccess)
            return false;

        IO::Path temporaryPath = path;
        temporaryPath.AsBaseString() += Fmt::FixedFormat(".{}.tmp", Uuid::Random());

        {
            auto stream = IO::FileStream::Open(temporaryPath, IO::OpenMode::kCreate);
            if (!stream)
                return false;

            const bool isWritten = stream.value()->WriteFromBuffer(bytes.data(), bytes.size()) == bytes.size();
            stream.value()->Close();
            if (!isWritten)
            {
                IO::File::Delete(temporaryPath);
                return false;
            }
        }

        if (IO::File::Replace(temporaryPath, path) != IO::ResultCode::kSuccess)
        {
            IO::File::Delete(temporaryPath);
            return false;
        }

        return true;
    }
} // namespace FE::AssetBuilder
