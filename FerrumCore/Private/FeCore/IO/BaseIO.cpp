#include <FeCore/IO/BaseIO.h>
#include <FeCore/IO/BaseIOPrivate.h>
#include <FeCore/IO/FileStream.h>
#include <FeCore/IO/Platform/PlatformPath.h>

namespace FE::IO
{
    namespace
    {
        struct StandardFiles final
        {
            FileStream m_files[festd::to_underlying(StandardDescriptor::kCount)];

            StandardFiles()
            {
                for (uint32_t i = 0; i < festd::size(m_files); ++i)
                {
                    m_files[i].SetBufferAllocator(Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear));
                    m_files[i].Open(static_cast<StandardDescriptor>(i));
                }
            }
        };

        StandardFiles* GStandardFiles;
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_Assert(GStandardFiles == nullptr, "Standard files already initialized");
        GStandardFiles = Memory::New<StandardFiles>(allocator);
    }


    void Internal::Shutdown()
    {
        FE_Assert(GStandardFiles != nullptr, "Standard files not initialized");
        GStandardFiles->~StandardFiles();
        GStandardFiles = nullptr;
    }


    festd::string_view GetResultDesc(const ResultCode code)
    {
        switch (code)
        {
        case ResultCode::kSuccess:
            return "Success";
        case ResultCode::kCanceled:
            return "Operation was canceled";
        case ResultCode::kPermissionDenied:
            return "Permission denied";
        case ResultCode::kNoFileOrDirectory:
            return "No such file or directory";
        case ResultCode::kFileExists:
            return "File already exists";
        case ResultCode::kFileTooLarge:
            return "File is too large";
        case ResultCode::kFilenameTooLong:
            return "Filename is too long";
        case ResultCode::kNotDirectory:
            return "Not a directory";
        case ResultCode::kIsDirectory:
            return "Is a directory";
        case ResultCode::kDirectoryNotEmpty:
            return "Directory is not empty";
        case ResultCode::kTooManyOpenFiles:
            return "Too many files are open";
        case ResultCode::kInvalidSeek:
            return "Invalid seek operation";
        case ResultCode::kIOError:
            return "IO error";
        case ResultCode::kDeadLock:
            return "Resource deadlock would occur";
        case ResultCode::kNotSupported:
            return "Operation is not supported";
        case ResultCode::kInvalidArgument:
            return "Argument value has not been accepted";
        case ResultCode::kInvalidFormat:
            return "Invalid file format";
        case ResultCode::kDecompressionError:
            return "Block file decompression failed";
        case ResultCode::kUnknownError:
        default:
            return "Unknown error";
        }
    }


    size_t PrintTo(const StandardDescriptor destination, const festd::string_view message)
    {
        FileStream& stream = GStandardFiles->m_files[festd::to_underlying(destination)];
        return stream.WriteFromBuffer(message.data(), message.size());
    }


    void Flush(const StandardDescriptor descriptor)
    {
        FileStream& stream = GStandardFiles->m_files[festd::to_underlying(descriptor)];
        stream.FlushWrites();
    }


    void Internal::FormatBufferFileAdapter::append(const char* str, const uint32_t length)
    {
        auto* stream = static_cast<FileStream*>(m_data);
        m_bytesWritten += stream->WriteFromBuffer(str, length);
    }


    Internal::FormatBufferFileAdapter Internal::FormatBufferFileAdapter::Create(const StandardDescriptor descriptor)
    {
        return FormatBufferFileAdapter{ .m_data = &GStandardFiles->m_files[festd::to_underlying(descriptor)] };
    }


    ResultCode Directory::TraverseRecursively(const festd::string_view path, const festd::string_view pattern,
                                              const festd::fixed_function<48, bool(const DirectoryEntry&)>& f)
    {
        FE_PROFILER_ZONE_TEXT("%.*s", path.size(), path.data());

        Platform::DirectoryIterationParams params;
        params.m_path = GetAbsolutePath(path);
        params.m_pattern = pattern;
        params.m_callbackData = reinterpret_cast<uintptr_t>(&f);
        params.m_callback = [](const uintptr_t callbackData, const DirectoryEntry& entry) {
            return (*reinterpret_cast<const std::decay_t<decltype(f)>*>(callbackData))(entry);
        };

        return Platform::IterateDirectoryRecursively(params);
    }
} // namespace FE::IO
