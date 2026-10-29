#pragma once
#include <Core/IO/Path.h>
#include <Core/Memory/RefCount.h>
#include <festd/vector.h>

namespace FE::IO
{
    //! @brief Position in a volume change journal. Persist it to continue reading after a restart.
    struct ChangeJournalCursor final
    {
        FE_RTTI_Reflect("6A27F84C-2E95-4E6C-8E77-98CDB040F61E");
        FE_RTTI_Serialize();

        //! Identity of the journal instance; it changes when the journal is deleted and recreated.
        uint64_t m_journalId = 0;

        //! First record that has not been read yet.
        int64_t m_nextRecord = 0;
    };


    enum class ChangeJournalReadResult : uint8_t
    {
        kSuccess,

        //! The journal was recreated, or discarded records the cursor had not read yet. The cursor now points at the end of the
        //! journal, and callers must treat every watched file as changed.
        kCursorExpired,

        kError,
    };


    //! @brief Reports file changes under watched directories from the change journal of their volume (the NTFS and ReFS USN
    //! journal on Windows).
    //!
    //! Unlike IO::DirectoryWatcher, the journal outlives the process: reading from a saved cursor reports everything that changed
    //! while nothing was running. Only changes of files that were closed are reported, so files that are still being written are
    //! skipped. Reads never block. Not thread-safe.
    struct ChangeJournal : public Memory::RefCountedObjectBase
    {
        //! @brief Open the journal of the volume that contains a directory. Returns null if the volume has no readable journal.
        static Rc<ChangeJournal> Open(festd::string_view directory);

        //! @brief Report changes anywhere in a directory tree on the journal's volume. Returns false if it cannot be watched.
        //!
        //! Subdirectories created later are tracked as their creation is read.
        virtual bool AddDirectory(festd::string_view directory) = 0;

        //! @brief Get a cursor at the end of the journal, which reports only changes made from now on.
        [[nodiscard]] virtual ChangeJournalCursor GetEndCursor() = 0;

        //! @brief Append normalized paths of watched files created, modified, renamed or deleted after the cursor, and advance it.
        virtual ChangeJournalReadResult ReadChanges(ChangeJournalCursor& cursor, festd::vector<Path>& changedFiles) = 0;
    };
} // namespace FE::IO
