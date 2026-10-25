#pragma once
#include <Core/IO/Path.h>
#include <Core/Memory/RefCount.h>
#include <festd/vector.h>

namespace FE::IO
{
    //! @brief Reports files that changed anywhere under a directory tree.
    //!
    //! Intended for development tools such as hot reloading. The operating system collects changes between polls, and a poll
    //! never blocks. Poll from threads that live as long as the watcher: a pending request is canceled when the thread that
    //! issued it exits.
    struct DirectoryWatcher : public Memory::RefCountedObjectBase
    {
        //! @brief Start watching a directory recursively. Returns null if the directory cannot be watched.
        static Rc<DirectoryWatcher> Create(festd::string_view directory);

        //! @brief Append the absolute paths of files created, modified, renamed or deleted since the previous poll.
        //!
        //! Paths are normalized as by IO::NormalizePath and start with the normalized watched directory.
        //!
        //! @return False if the system dropped notifications, e.g. after a burst of changes. Callers should then treat every
        //!         file under the directory as changed.
        virtual bool PollChanges(festd::vector<Path>& changedFiles) = 0;
    };
} // namespace FE::IO
