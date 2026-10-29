#include <Core/Base/PlatformInclude.h>
#include <Core/IO/ChangeJournal.h>
#include <Core/Logging/Trace.h>
#include <Core/Math/Uuid.h>
#include <Core/Platform/Windows/Common.h>
#include <Core/Strings/Encoding.h>
#include <festd/unordered_map.h>

// PlatformInclude.h removes the near and far macros, which winioctl.h still uses.
#define near
#define far
#include <winioctl.h>
#undef near
#undef far

namespace FE::IO
{
    namespace
    {
        constexpr uint32_t kReadBufferSize = 64 * 1024;
        constexpr uint32_t kDirectoryBufferSize = 64 * 1024;


        //! FILE_ID_128 as a hashable key. NTFS 64-bit file reference numbers occupy the low half.
        Uuid MakeFileId(const void* bytes, const uint32_t byteSize)
        {
            Uuid result = Uuid::kNull;
            memcpy(result.m_bytes, bytes, byteSize);
            return result;
        }


        HANDLE OpenDirectory(const Path& path)
        {
            const Str::Utf8ToUtf16 widePath{ path.data(), path.size() };
            return CreateFileW(widePath.ToWideString(),
                               FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS,
                               nullptr);
        }


        //! The fields of USN_RECORD_V2 and USN_RECORD_V3 used here.
        struct JournalRecord final
        {
            Uuid m_fileId;
            Uuid m_parentId;
            DWORD m_reason = 0;
        };


        bool ParseRecord(const USN_RECORD_COMMON_HEADER* header, JournalRecord& record)
        {
            if (header->MajorVersion == 2)
            {
                const auto* recordV2 = reinterpret_cast<const USN_RECORD_V2*>(header);
                record.m_fileId = MakeFileId(&recordV2->FileReferenceNumber, sizeof(recordV2->FileReferenceNumber));
                record.m_parentId = MakeFileId(&recordV2->ParentFileReferenceNumber, sizeof(recordV2->ParentFileReferenceNumber));
                record.m_reason = recordV2->Reason;
                return true;
            }

            if (header->MajorVersion == 3)
            {
                const auto* recordV3 = reinterpret_cast<const USN_RECORD_V3*>(header);
                record.m_fileId = MakeFileId(&recordV3->FileReferenceNumber, sizeof(recordV3->FileReferenceNumber));
                record.m_parentId = MakeFileId(&recordV3->ParentFileReferenceNumber, sizeof(recordV3->ParentFileReferenceNumber));
                record.m_reason = recordV3->Reason;
                return true;
            }

            return false;
        }


        //! A file or directory in a watched tree. Roots store their absolute path as the name and have no parent.
        struct TrackedFile final
        {
            Uuid m_parentId;
            Path m_name;
        };


        //! Reads the USN journal through a handle to the volume root directory.
        //!
        //! Unprivileged journal reads omit file names. Every file in the watched trees is therefore tracked by its file ID, which
        //! gives the names of deleted files. Names of new and renamed files are looked up by opening the file by its ID.
        struct WindowsChangeJournal final : public ChangeJournal
        {
            explicit WindowsChangeJournal(const HANDLE volume)
                : m_volume(volume)
            {
                m_buffer.resize(kReadBufferSize / sizeof(USN));
            }

            ~WindowsChangeJournal() override
            {
                CloseHandle(m_volume);
            }

            bool QueryJournal(USN_JOURNAL_DATA_V0& data) const
            {
                DWORD bytesReturned = 0;
                return DeviceIoControl(m_volume,
                                       FSCTL_QUERY_USN_JOURNAL,
                                       nullptr,
                                       0,
                                       &data,
                                       sizeof(data),
                                       &bytesReturned,
                                       nullptr);
            }

            bool AddDirectory(const festd::string_view directory) override
            {
                FE_PROFILER_ZONE();

                const Path root = NormalizePath(directory);
                const HANDLE handle = OpenDirectory(root);
                if (handle == INVALID_HANDLE_VALUE)
                    return false;

                FILE_ID_INFO info{};
                const bool hasId = GetFileInformationByHandleEx(handle, FileIdInfo, &info, sizeof(info));
                CloseHandle(handle);
                if (!hasId)
                    return false;

                const Uuid rootId = MakeFileId(&info.FileId, sizeof(info.FileId));
                m_files.insert_or_assign(rootId, TrackedFile{ Uuid::kNull, root });
                return TrackDirectoryContents(rootId, root);
            }

            ChangeJournalCursor GetEndCursor() override
            {
                USN_JOURNAL_DATA_V0 data{};
                if (!QueryJournal(data))
                    return {};

                return ChangeJournalCursor{ data.UsnJournalID, data.NextUsn };
            }

            ChangeJournalReadResult ReadChanges(ChangeJournalCursor& cursor, festd::vector<Path>& changedFiles) override
            {
                FE_PROFILER_ZONE();

                USN_JOURNAL_DATA_V0 data{};
                if (!QueryJournal(data))
                {
                    Logger::LogError("Failed to query the change journal: {}", GetLastError());
                    return ChangeJournalReadResult::kError;
                }

                const bool isExpired = cursor.m_journalId != data.UsnJournalID || cursor.m_nextRecord < data.FirstUsn;
                if (isExpired)
                {
                    cursor = ChangeJournalCursor{ data.UsnJournalID, data.NextUsn };
                    return ChangeJournalReadResult::kCursorExpired;
                }

                READ_USN_JOURNAL_DATA_V1 readData{};
                readData.StartUsn = cursor.m_nextRecord;
                readData.ReasonMask = 0xffffffff;
                readData.ReturnOnlyOnClose = TRUE;
                readData.UsnJournalID = data.UsnJournalID;
                readData.MinMajorVersion = 2;
                readData.MaxMajorVersion = 3;

                for (;;)
                {
                    DWORD bytesReturned = 0;
                    const bool succeeded = DeviceIoControl(m_volume,
                                                           FSCTL_READ_UNPRIVILEGED_USN_JOURNAL,
                                                           &readData,
                                                           sizeof(readData),
                                                           m_buffer.data(),
                                                           m_buffer.size() * sizeof(USN),
                                                           &bytesReturned,
                                                           nullptr);
                    if (!succeeded)
                    {
                        const DWORD error = GetLastError();
                        if (error == ERROR_JOURNAL_ENTRY_DELETED || error == ERROR_JOURNAL_DELETE_IN_PROGRESS)
                        {
                            cursor = GetEndCursor();
                            return ChangeJournalReadResult::kCursorExpired;
                        }

                        Logger::LogError("Failed to read the change journal: {}", error);
                        return ChangeJournalReadResult::kError;
                    }

                    if (bytesReturned < sizeof(USN))
                        break;

                    AppendChanges(festd::span{ reinterpret_cast<const std::byte*>(m_buffer.data()), bytesReturned },
                                  changedFiles);

                    const USN nextRecord = m_buffer[0];
                    if (nextRecord == readData.StartUsn)
                        break;

                    readData.StartUsn = nextRecord;
                }

                cursor.m_nextRecord = readData.StartUsn;
                return ChangeJournalReadResult::kSuccess;
            }

        private:
            //! Track every file under a directory, using one directory enumeration per directory instead of opening each file.
            bool TrackDirectoryContents(const Uuid directoryId, const Path& directory)
            {
                const HANDLE handle = OpenDirectory(directory);
                if (handle == INVALID_HANDLE_VALUE)
                    return false;

                festd::inline_vector<std::pair<Uuid, Path>, 16> subdirectories;
                festd::vector<uint64_t> buffer;
                buffer.resize(kDirectoryBufferSize / sizeof(uint64_t));

                bool isFirstQuery = true;
                for (;;)
                {
                    const FILE_INFO_BY_HANDLE_CLASS infoClass =
                        isFirstQuery ? FileIdExtdDirectoryRestartInfo : FileIdExtdDirectoryInfo;
                    isFirstQuery = false;

                    if (!GetFileInformationByHandleEx(handle, infoClass, buffer.data(), buffer.size() * sizeof(uint64_t)))
                        break;

                    const auto* bytes = reinterpret_cast<const std::byte*>(buffer.data());
                    for (;;)
                    {
                        const auto* entry = reinterpret_cast<const FILE_ID_EXTD_DIR_INFO*>(bytes);
                        const festd::span name{ entry->FileName, static_cast<uint32_t>(entry->FileNameLength / sizeof(WCHAR)) };
                        const bool isDotEntry =
                            (name.size() == 1 && name[0] == L'.') || (name.size() == 2 && name[0] == L'.' && name[1] == L'.');
                        if (!isDotEntry)
                        {
                            const Uuid fileId = MakeFileId(&entry->FileId, sizeof(entry->FileId));
                            const Path fileName = Platform::ConvertWideString<Path>(name);
                            if ((entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                                subdirectories.push_back({ fileId, directory / fileName });

                            m_files.insert_or_assign(fileId, TrackedFile{ directoryId, fileName });
                        }

                        if (entry->NextEntryOffset == 0)
                            break;

                        bytes += entry->NextEntryOffset;
                    }
                }

                const bool isComplete = GetLastError() == ERROR_NO_MORE_FILES;
                CloseHandle(handle);

                bool succeeded = isComplete;
                for (const auto& [subdirectoryId, subdirectory] : subdirectories)
                    succeeded &= TrackDirectoryContents(subdirectoryId, subdirectory);

                return succeeded;
            }

            //! Build the current path of a tracked file from its ancestors.
            [[nodiscard]] Path GetTrackedPath(const Uuid fileId) const
            {
                festd::inline_vector<const Path*, 16> names;
                Uuid currentId = fileId;
                while (currentId != Uuid::kNull)
                {
                    const auto iter = m_files.find(currentId);
                    if (iter == m_files.end())
                        return {};

                    names.push_back(&iter->second.m_name);
                    currentId = iter->second.m_parentId;
                }

                if (names.empty())
                    return {};

                Path result = *names.back();
                for (uint32_t index = names.size() - 1; index > 0; --index)
                    result /= festd::string_view{ names[index - 1]->data(), names[index - 1]->size() };

                return result;
            }

            //! Open a file by its ID and get its file name, or an empty path if it no longer exists.
            [[nodiscard]] Path GetCurrentName(const Uuid fileId) const
            {
                FILE_ID_DESCRIPTOR descriptor{};
                descriptor.dwSize = sizeof(descriptor);
                descriptor.Type = ExtendedFileIdType;
                memcpy(&descriptor.ExtendedFileId, fileId.m_bytes, sizeof(descriptor.ExtendedFileId));

                const HANDLE handle = OpenFileById(m_volume,
                                                   &descriptor,
                                                   FILE_READ_ATTRIBUTES,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                   nullptr,
                                                   FILE_FLAG_BACKUP_SEMANTICS);
                if (handle == INVALID_HANDLE_VALUE)
                    return {};

                WCHAR fullPath[MAX_PATH * 2] = {};
                const DWORD length = GetFinalPathNameByHandleW(handle, fullPath, MAX_PATH * 2, FILE_NAME_NORMALIZED);
                CloseHandle(handle);
                if (length == 0 || length >= MAX_PATH * 2)
                    return {};

                // Only the file name is used, so the "\\?\" prefix of the final path does not matter.
                const WCHAR* nameStart = fullPath + length;
                while (nameStart > fullPath && nameStart[-1] != L'\\')
                    --nameStart;

                const uint32_t nameLength = static_cast<uint32_t>(fullPath + length - nameStart);
                return Platform::ConvertWideString<Path>(festd::span{ nameStart, nameLength });
            }

            void AppendChanges(const festd::span<const std::byte> bytes, festd::vector<Path>& changedFiles)
            {
                uint32_t offset = sizeof(USN);
                while (offset + sizeof(USN_RECORD_COMMON_HEADER) <= bytes.size())
                {
                    const auto* header = reinterpret_cast<const USN_RECORD_COMMON_HEADER*>(bytes.data() + offset);
                    if (header->RecordLength == 0)
                        break;

                    offset += header->RecordLength;

                    JournalRecord record;
                    if (ParseRecord(header, record))
                        ApplyRecord(record, changedFiles);
                }
            }

            void ApplyRecord(const JournalRecord& record, festd::vector<Path>& changedFiles)
            {
                const bool isInWatchedTree = m_files.contains(record.m_parentId);
                const auto tracked = m_files.find(record.m_fileId);
                const bool isTracked = tracked != m_files.end();
                if (!isInWatchedTree && !isTracked)
                    return;

                // The previous location of a tracked file changed too, whether the file was deleted, renamed or moved away.
                Path previousPath;
                if (isTracked)
                    previousPath = GetTrackedPath(record.m_fileId);

                const bool isDeleted = (record.m_reason & USN_REASON_FILE_DELETE) != 0;
                Path currentName;
                if (isInWatchedTree && !isDeleted)
                    currentName = GetCurrentName(record.m_fileId);

                if (currentName.empty())
                {
                    if (isTracked)
                        m_files.erase(record.m_fileId);
                }
                else
                {
                    m_files.insert_or_assign(record.m_fileId, TrackedFile{ record.m_parentId, currentName });

                    const Path currentPath = GetTrackedPath(record.m_fileId);
                    if (currentPath != previousPath)
                        changedFiles.push_back(currentPath);
                }

                if (!previousPath.empty())
                    changedFiles.push_back(std::move(previousPath));
            }

            void DoRelease() override
            {
                Memory::DefaultDelete(this);
            }

            HANDLE m_volume = INVALID_HANDLE_VALUE;
            festd::vector<USN> m_buffer;
            festd::unordered_dense_map<Uuid, TrackedFile> m_files;
        };
    } // namespace


    Rc<ChangeJournal> ChangeJournal::Open(const festd::string_view directory)
    {
        FE_PROFILER_ZONE();

        const Path absoluteDirectory = GetAbsolutePath(directory);
        const Str::Utf8ToUtf16 widePath{ absoluteDirectory.data(), absoluteDirectory.size() };

        WCHAR volumePath[MAX_PATH + 1] = {};
        if (!GetVolumePathNameW(widePath.ToWideString(), volumePath, MAX_PATH))
            return nullptr;

        // Without administrator rights the volume device cannot be opened, but the unprivileged journal requests accept a
        // handle to any directory on the volume. The root directory is used, since it outlives every watched directory.
        const HANDLE volume = CreateFileW(volumePath,
                                          FILE_READ_ATTRIBUTES,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                          nullptr,
                                          OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS,
                                          nullptr);
        if (volume == INVALID_HANDLE_VALUE)
            return nullptr;

        Rc journal = Memory::DefaultNew<WindowsChangeJournal>(volume);
        USN_JOURNAL_DATA_V0 data{};
        if (!journal->QueryJournal(data))
            return nullptr;

        return journal;
    }
} // namespace FE::IO
