#pragma once

#include "Containers/IDVector.h"
#include "Containers/THashTable.h"
#include "ECS/Job.h"
#include "Utilities/IDGenerator.h"

#include <array>
#include <condition_variable>
#include <mutex>
#include <unordered_set>

namespace LambdaEngine
{
    class JobScheduler
    {
    public:
        JobScheduler();
        ~JobScheduler() = default;

        void Tick();

        void ScheduleJob(const Job& job, uint32_t phase);
        void ScheduleJobASAP(const Job& job);
        void ScheduleJobs(const TArray<Job>& jobs, uint32_t phase);

        /*  scheduleRegularJob schedules a job that is performed each frame, until it is explicitly deregistered using the job ID.
            Returns the job ID. */
        uint32 ScheduleRegularJob(const Job& job, uint32_t phase);
        void DescheduleRegularJob(uint32 phase, uint32 jobID);

        // Schedules an advance to the next phase
        void NextPhase();

    private:
        const Job* FindExecutableJob();
        bool CanExecute(const Job& job) const;

        // Register the component accesses (reads and writes) to be performed by the job
        void RegisterJobExecution(const Job& job);
        void DeregisterJobExecution(const Job& job);

        void SetPhase(uint32_t phase);

        // Checks if there are more jobs to execute in the current phase
        bool PhaseJobsExist() const;

    private:
        // One vector of jobs per phase. These jobs are anything but system ticks, i.e. they are irregularly scheduled.
        // g_PhaseCount + 1 is used to allow jobs to be scheduled as post-systems
        std::array<TArray<Job>, g_PhaseCount + 1> m_Jobs;

        // Indices to jobs yet to be executed within each phase
        std::array<TArray<uint32>, g_PhaseCount + 1> m_JobIndices;

        // Regular jobs are performed each frame, until they are explicitly deregistered
        std::array<IDDVector<Job>, g_PhaseCount> m_RegularJobs;
        IDGenerator m_RegularJobIDGenerator;

        // IDs of the systems to tick in the current phase
        // Populated at the beginning of each phase
        TArray<uint32> m_RegularJobIDsToTick;

        // Maps component TIDs to the amount of jobs are reading from them.
        // A zero read count means the component type is being written to.
        THashTable<std::type_index, uint32> m_ProcessingComponents;

        // Used to join threads executing jobs once the current phase's jobs have all been scheduled
        TArray<uint32> m_ThreadHandles;

        std::mutex m_Lock;
        std::condition_variable m_ScheduleTimeoutCvar;

        uint32_t m_CurrentPhase;
    };
}
