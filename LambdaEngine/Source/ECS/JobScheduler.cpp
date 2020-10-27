#include "ECS/JobScheduler.h"

#include "ECS/ECSCore.h"
#include "ECS/EntitySubscriber.h"
#include "Threading/API/ThreadPool.h"

#include <numeric>

namespace LambdaEngine
{
    JobScheduler::JobScheduler() :
            m_CurrentPhase(0u)
        ,   m_DeltaTime(0.0f)
    {}

    void JobScheduler::Tick(float32 dt)
    {
        m_DeltaTime = dt;

        std::unique_lock<std::mutex> uLock(m_Lock);
        SetPhase(0u);
        AccumulateRegularJobs();

        // m_CurrentPhase == PHASE_COUNT means all regular jobs are finished, and only post-systems jobs are executed
        while (m_CurrentPhase <= PHASE_COUNT)
        {
            const Job* pJob = nullptr;
            do
            {
                pJob = FindExecutableJob();
                if (pJob)
                {
                    RegisterJobExecution(*pJob);
                    m_ThreadHandles.PushBack(ThreadPool::Execute(std::bind_front(&JobScheduler::ExecuteJob, this, *pJob)));
                }
            } while(pJob);

            if (!PhaseJobsExist())
            {
                // No more jobs in the current phase, perhaps currently running jobs will schedule new ones
                m_Lock.unlock();
                for (uint32 threadHandle : m_ThreadHandles)
                {
                    ThreadPool::Join(threadHandle);
                }

                m_ThreadHandles.Clear();
                m_Lock.lock();

                if (!PhaseJobsExist())
                {
                    NextPhase();
                }
            }
            else
            {
                m_ScheduleTimeoutCvar.wait(uLock);
            }
        }
    }

    void JobScheduler::ScheduleJob(const Job& job, uint32_t phase)
    {
        std::scoped_lock<std::mutex> lock(m_Lock);
        m_Jobs[phase].EmplaceBack(job);
        m_JobIndices[phase].PushBack(m_Jobs[phase].GetSize() - 1u);

        m_ScheduleTimeoutCvar.notify_all();
    }

    void JobScheduler::ScheduleJobASAP(const Job& job)
    {
        std::scoped_lock<std::mutex> lock(m_Lock);
        const uint32 phase = m_CurrentPhase >= m_Jobs.size() ? 0u : m_CurrentPhase;
        m_Jobs[phase].EmplaceBack(job);
        m_JobIndices[phase].PushBack(m_Jobs[phase].GetSize() - 1u);

        m_ScheduleTimeoutCvar.notify_all();
    }

    void JobScheduler::ScheduleJobs(const TArray<Job>& jobs, uint32_t phase)
    {
        std::scoped_lock<std::mutex> lock(m_Lock);

        // Push jobs
        const uint32 oldJobsCount = m_Jobs[phase].GetSize();
        m_Jobs[phase].Resize(oldJobsCount + jobs.GetSize());
        std::copy_n(jobs.begin(), jobs.GetSize(), &m_Jobs[phase][oldJobsCount]);

        // Push job indices
        TArray<uint32>& jobIndices = m_JobIndices[phase];
        const uint32 oldIndicesCount = m_JobIndices[phase].GetSize();
        m_JobIndices[phase].Resize(oldIndicesCount + jobs.GetSize());
        std::iota(&jobIndices[oldIndicesCount - 1u], &jobIndices.GetBack(), oldJobsCount);

        m_ScheduleTimeoutCvar.notify_all();
    }

    uint32 JobScheduler::ScheduleRegularJob(const RegularJob& job, uint32_t phase)
    {
        std::scoped_lock<std::mutex> lock(m_Lock);

        const uint32 jobID = m_RegularJobIDGenerator.GenID();
        m_RegularJobs[phase].PushBack(job, jobID);

        return jobID;
    }

    void JobScheduler::DescheduleRegularJob(uint32 phase, uint32 jobID)
    {
        std::scoped_lock<std::mutex> lock(m_Lock);
        m_RegularJobs[phase].Pop(jobID);
    }

    const Job* JobScheduler::FindExecutableJob()
    {
        const TArray<Job>& scheduledJobs = m_Jobs[m_CurrentPhase];
        TArray<uint32>& jobIndices = m_JobIndices[m_CurrentPhase];

        for (uint32& jobIndex : jobIndices)
        {
            const Job& job = scheduledJobs[jobIndex];
            if (CanExecute(job))
            {
                jobIndex = jobIndices.GetBack();
                jobIndices.PopBack();

                return &job;
            }
        }

        if (m_CurrentPhase < PHASE_COUNT && !m_RegularJobsToAccumulate[m_CurrentPhase].empty())
        {
            TArray<uint32>& regularJobIDsToTick = m_RegularJobIDsToTick[m_CurrentPhase];
            for (uint32& jobID : regularJobIDsToTick)
            {
                RegularJob& job = m_RegularJobs[m_CurrentPhase].IndexID(jobID);

                if (CanExecute(job))
                {
                    job.Accumulator -= job.TickPeriod;
                    if (job.TickPeriod <= 0.0f || job.Accumulator < job.TickPeriod)
                    {
                        m_RegularJobsToAccumulate[m_CurrentPhase].erase(jobID);
                    }

                    jobID = regularJobIDsToTick.GetBack();
                    regularJobIDsToTick.PopBack();

                    return &job;
                }
            }
        }

        return nullptr;
    }

    bool JobScheduler::CanExecute(const Job& job) const
    {
        // Prevent multiple jobs from accessing the same components where at least one of them has write permissions
        // i.e. prevent data races
        for (const ComponentAccess& componentReg : job.Components)
        {
            auto processingComponentItr = m_ProcessingComponents.find(componentReg.pTID);
            /*  processingComponentItr->second is the amount of jobs currently reading from the component type.
                If the count is zero, a job is writing to the component type. */
            if (processingComponentItr != m_ProcessingComponents.end() && (componentReg.Permissions == RW || processingComponentItr->second == 0))
            {
                return false;
            }
        }

        return true;
    }

    bool JobScheduler::CanExecuteRegularJob(const RegularJob& job) const
    {
        const Job& simpleJob = job;
        return job.Accumulator >= job.TickPeriod && CanExecute(simpleJob);
    }

    void JobScheduler::ExecuteJob(Job job)
    {
        job.Function();
        m_Lock.lock();
        DeregisterJobExecution(job);
        m_Lock.unlock();
    }

    void JobScheduler::RegisterJobExecution(const Job& job)
    {
        for (const ComponentAccess& componentReg : job.Components)
        {
            const uint32 isReadOnly = componentReg.Permissions == R;

            auto processingComponentItr = m_ProcessingComponents.find(componentReg.pTID);
            if (processingComponentItr == m_ProcessingComponents.end())
            {
                m_ProcessingComponents.insert({componentReg.pTID, isReadOnly});
            }
            else
            {
                processingComponentItr->second += isReadOnly;
            }
        }
    }

    void JobScheduler::DeregisterJobExecution(const Job& job)
    {
        for (const ComponentAccess& componentReg : job.Components)
        {
            auto processingComponentItr = m_ProcessingComponents.find(componentReg.pTID);
            if (processingComponentItr->second <= 1u)
            {
                // The job was the only one reading or writing to the component type
                m_ProcessingComponents.erase(processingComponentItr);
            }
            else
            {
                processingComponentItr->second -= 1u;
            }
        }

        m_ScheduleTimeoutCvar.notify_all();
    }

    void JobScheduler::SetPhase(uint32_t phase)
    {
        if (m_CurrentPhase <= PHASE_COUNT)
        {
            m_Jobs[m_CurrentPhase].Clear();
        }

        m_CurrentPhase = phase;
    }

    void JobScheduler::NextPhase()
    {
        uint32 upcomingPhase = m_CurrentPhase + 1u;
        // All phases have been processed. Some might need to be replayed to accumulate regular jobs
        if (m_CurrentPhase == PHASE_COUNT)
        {
            for (uint32 phase = 0; phase < PHASE_COUNT; phase++)
            {
                const std::unordered_set<uint32>& regularJobsToAccumulate = m_RegularJobsToAccumulate[phase];
                if (!regularJobsToAccumulate.empty())
                {
                    // Find which regular jobs need accumulating
                    TArray<uint32>& regularJobIDsToTick = m_RegularJobIDsToTick[phase];
                    for (uint32 jobID : regularJobsToAccumulate)
                    {
                        regularJobIDsToTick.PushBack(jobID);
                    }

                    upcomingPhase = phase;
                    break;
                }
            }
        }

        ECSCore* pECS = ECSCore::GetInstance();
        pECS->PerformComponentRegistrations();
        pECS->PerformComponentDeletions();
        pECS->PerformEntityDeletions();
        SetPhase(upcomingPhase);
    }

    void JobScheduler::AccumulateRegularJobs()
    {
        for (uint32 phase = 0; phase < PHASE_COUNT; phase++)
        {
            IDDVector<RegularJob>& regularJobs = m_RegularJobs[phase];
            const TArray<uint32>& jobIDs = regularJobs.GetIDs();
            TArray<uint32>& regularJobIDsToTick = m_RegularJobIDsToTick[phase];

            for (uint32 jobIdx = 0; jobIdx < regularJobs.Size(); jobIdx++)
            {
                RegularJob& regularJob = regularJobs[jobIdx];
                // Increase the accumulator only if the regular job has a non-zero tick period
                regularJob.Accumulator += m_DeltaTime * (regularJob.TickPeriod > 0.0f);
                if (regularJob.Accumulator >= regularJob.TickPeriod)
                {
                    m_RegularJobsToAccumulate[phase].insert(jobIDs[jobIdx]);
                    regularJobIDsToTick.PushBack(jobIDs[jobIdx]);
                }
            }
        }
    }

    bool JobScheduler::PhaseJobsExist() const
    {
        return (m_CurrentPhase < PHASE_COUNT && !m_RegularJobIDsToTick[m_CurrentPhase].IsEmpty()) || !m_JobIndices[m_CurrentPhase].IsEmpty();
    }
}
