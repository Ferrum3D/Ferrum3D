#pragma once
#include <mutex>
#include <vector>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include "CoreUtils.h"
#include "FeStackAllocator.h"

namespace Ferrum
{
	FE_ENUM(FeJobType)
	{
		/**
		 * @brief A job that's working with the hard drive
		*/
		HardDriveJob = 1 << 0,

		/**
		 * @brief A heavy job that's executing during multiple frames
		*/
		HeavyJob = 1 << 1,

		/**
		 * @brief A job that should be completed within the current frame
		*/
		SingleFrameJob = 1 << 2,

		/**
		 * @brief A light job that's execution time is much less than an average frame time
		*/
		LightJob = 1 << 3,

		Count = 4
	};

	typedef void(*FeJobFunction)(void*);

	struct FeJobHandle
	{
	private:
		friend class FeJobSystem;
		friend class FeWorkerThread;
		friend class FeJobQueue;

		FeJobType m_Type;

		std::condition_variable* m_JobCv{};
		std::mutex* m_JobMutex{};
		void* m_Instance{};

		FeJobFunction m_Func;

		bool* m_Ready{};

		inline FeJobHandle(FeJobFunction func, FeJobType type, void* inst) {
			m_Func = func;
			m_Type = type;
			m_JobCv = new std::condition_variable();
			m_JobMutex = new std::mutex();
			m_Ready = new bool();
			m_Instance = inst;
		}

		inline void MarkCompleted() {
			std::unique_lock lk(*m_JobMutex);
			*m_Ready = true;
			m_JobCv->notify_all();
		}

	public:
		inline FeJobType GetType() const {
			return m_Type;
		}

		inline void Wait() {
			{
				std::unique_lock lk(*m_JobMutex);
				while (!*m_Ready) m_JobCv->wait(lk);
			}
			delete m_JobCv;
			delete m_JobMutex;
		}
	};

	class FeJobQueue
	{
		std::shared_mutex m_Mutex{};
		std::queue<FeJobHandle> m_Queues[(int)FeJobType::Count];

	public:
		inline bool HasJobs(FeJobType flags) {
			std::shared_lock lk(m_Mutex);
			for (int i = 0; i < (int)FeJobType::Count; ++i) {
				auto flag = 1 << i;
				if (int(flags & flag) && !m_Queues[i].empty()) {
					return true;
				}
			}

			return false;
		}

		inline bool TryPopJob(FeJobType flags, FeJobHandle& handle) {
			std::unique_lock lk(m_Mutex);
			handle.m_Func = nullptr;
			for (int i = 0; i < (int)FeJobType::Count; ++i) {
				auto flag = 1 << i;
				if ((int(flags) & flag) && !m_Queues[i].empty()) {
					handle = m_Queues[i].front();
					m_Queues[i].pop();
					break;
				}
			}

			return handle.m_Func;
		}

		inline void Enqueue(const FeJobHandle& handle) {
			std::unique_lock lk(m_Mutex);
			m_Queues[FeCountTrailingZeros((uint32_t)handle.GetType())].push(handle);
		}
	};

	enum class FeWorkerThreadState
	{
		None,

		/**
		 * @brief Thread is working
		*/
		Working,

		/**
		 * @brief Stop requested, but thread is allowed to finish all jobs in queue
		*/
		JoinRequested,

		/**
		 * @brief Stop requested, thread must stop working as soon as possible
		*/
		StopRequested
	};

	struct FeWorkerThreadDesc
	{
		FeJobType Flags;
		uint32_t Id;
		std::string Name;
		std::mutex* JobSystemMutex;
		std::condition_variable* JobSystemCv;
		FeJobQueue* JobSystemQueue;
	};

	class FeWorkerThread
	{
		inline void ThreadLoop() {
			while (true) {
				{
					std::unique_lock lk(JobSystemMutex);

					JobSystemCv.wait(lk, [this]() {
						return JobSystemQueue.HasJobs(Flags)
							|| State == FeWorkerThreadState::JoinRequested
							|| State == FeWorkerThreadState::StopRequested;
						});

					if ((State == FeWorkerThreadState::JoinRequested && !JobSystemQueue.HasJobs(Flags))
						|| State == FeWorkerThreadState::StopRequested)
						break;
				}

				FeJobHandle job{ nullptr, Flags, nullptr };
				if (JobSystemQueue.TryPopJob(Flags, job)) {
					job.m_Func(job.m_Instance);
					job.MarkCompleted();
				}
			}

			State = FeWorkerThreadState::None;
		}

	public:
		std::thread Handle;
		FeJobType Flags;
		uint32_t Id;
		std::string Name;
		std::mutex& JobSystemMutex;
		std::condition_variable& JobSystemCv;
		FeJobQueue& JobSystemQueue;

		std::atomic<FeWorkerThreadState> State{};

		inline FeWorkerThread(const FeWorkerThreadDesc& desc) : JobSystemMutex(*desc.JobSystemMutex),
				JobSystemCv(*desc.JobSystemCv), JobSystemQueue(*desc.JobSystemQueue) {
			Name = desc.Name;
			Id = desc.Id;
			Flags = desc.Flags;

			Handle = std::thread(&FeWorkerThread::ThreadLoop, this);
		}
	};

	inline constexpr size_t FeSyncSizePerThread = sizeof(std::mutex) + sizeof(std::condition_variable);

	class FE_CORE_API FeJobSystem
	{
		std::vector<std::unique_ptr<FeWorkerThread>> m_Threads{};
		FeJobQueue m_Queue{};

		std::mutex m_Mutex{};
		std::condition_variable m_Cv{};

		FeStackAllocator m_SyncAllocator;

		FeJobHandle ScheduleJob(FeJobFunction job, FeJobType type, void* inst);

		void Init(const std::vector<FeJobType>& threadFlagArray);

	public:
		inline FeJobSystem(const std::vector<FeJobType>& threadFlagArray)
				: m_SyncAllocator(threadFlagArray.size() * FeSyncSizePerThread) {
			Init(threadFlagArray);
		}

		inline FeJobSystem(size_t threadCount)
				: m_SyncAllocator(threadCount * FeSyncSizePerThread) {
			std::vector<FeJobType> threadFlagArray(threadCount, FeJobType::LightJob | FeJobType::SingleFrameJob);
			threadFlagArray[0] |= FeJobType::HardDriveJob | FeJobType::HeavyJob;
			Init(threadFlagArray);
		}

		/**
		 * @brief Copy the Job struct to the worker thread and call Execute() in parallel
		 * @tparam Job Type of job struct, e.g.
		 * ```
		 * struct Job
		 * {
		 *     int JobData;
		 *     void Execute() { ... }
		 * };
		 * ```
		 * @param job An instance of job struct
		 * @param type Type of job
		 * @return A handle to the enqueued job
		*/
		template<class Job>
		inline FeJobHandle Schedule(const Job& job, FeJobType type) {
			auto func = [](void* ptr) {
				auto& instance = *((Job*)ptr);
				instance.Execute();
				delete &instance;
			};
			return ScheduleJob(func, type, new Job(job));
		}

		inline void JoinAll() {
			for (auto& t : m_Threads) {
				t->State = FeWorkerThreadState::JoinRequested;
			}
			m_Cv.notify_all();
			for (auto& t : m_Threads) {
				if (t->Handle.joinable())
					t->Handle.join();
			}
		}

		inline void ForceStop() {
			for (auto& t : m_Threads) {
				t->State = FeWorkerThreadState::StopRequested;
			}
			m_Cv.notify_all();
			for (auto& t : m_Threads) {
				if (t->Handle.joinable())
					t->Handle.join();
			}
		}
	};
}
