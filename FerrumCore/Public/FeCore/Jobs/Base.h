#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE
{
    //! \brief Describes the priority of a job.
    //!
    //! Jobs with different priorities are assigned to different queues.
    //! Each worker thread has a set of queues (one for each priority)
    //! for the jobs that can only run on that thread (due to affinity).
    //! There is also two global sets of queues: one for foreground jobs
    //! and one for background jobs.
    enum class JobPriority : uint16_t
    {
        kLow = 0,
        kNormal = 1,
        kHigh = 2,

        kCount = kHigh + 1,
    };


    //! \brief Specifies which threads a job is allowed to run on.
    //!
    //! There are two good reasons for this to exist:
    //! Firstly, some jobs can only safely run on a certain thread, e.g. Windows
    //! message loop jobs should only run on the main thread.
    //! Secondly, we have so-called "foreground" and "background" jobs.
    //! Foreground jobs must be executed within one frame, while background jobs
    //! can span multiple frames. While we can allow foreground jobs to preempt
    //! background jobs, we cannot allow background jobs to preempt foreground
    //! jobs. Thus, we need a separate foreground thread pool.
    //!
    //! In the JobSystem, each thread is assigned a unique ID. Foreground thread
    //! IDs are in range [0, 31] (MainThread being thread 0 - the first
    //! foreground thread), while background thread IDs are in range [32, 63].
    //! So, for instance, UINT32_MAX specifies an affinity mask that matches
    //! all foreground threads.
    //!
    //! Note: currently it is only possible to specify affinity either for
    //!   a single thread (e.g, main thread, worker5, etc.) or for an entire
    //!   thread pool (e.g., all foreground threads or all background threads).
    //!   Values like 0b11 are considered invalid.
    //!   So, we intentionally do not provide any bit operators as all
    //!   the correct combinations can be specified using the enum values
    //!   or IJobSystem::GetAffinityMaskForCurrentThread().
    enum class FiberAffinityMask : uint64_t
    {
        kNone = 0,
        kMainThread = 1,
        kAllForeground = 0x00000000ffffffff,
        kAllBackground = 0xffffffff00000000,
        kAll = kMainThread | kAllForeground | kAllBackground,
    };


    enum class JobThreadPoolType : uint32_t
    {
        kGeneric,
        kForeground,
        kBackground,
        kCount,
    };


    struct Job;
    struct IJobSystem;
    struct JobSystem;
    struct WaitGroup;
} // namespace FE
