#include "ECS/JobScheduler.h"

#include "ECS/EntitySubscriber.h"
#include "Threading/API/ThreadPool.h"

#include <numeric>

namespace LambdaEngine
{
    JobScheduler::JobScheduler()
        :m_CurrentPhase(0u)
    {}

    void JobScheduler::Tick()
    {
        std::unique_lock<std::mutex> uLock(m_Lock);
        SetPhase(0u);

        // m_CurrentPhase == g_PhaseCount means all sytems are finished, and only post-systems jobs are executed
        while (m_CurrentPhase <= g_PhaseCount)
        {
            const Job* pJob = nullptr;
            do
            {
                pJob = FindExecutableJob();
                if (pJob)
                {
                    RegisterJobExecution(*pJob);
                    m_ThreadHandles.PushBack(ThreadPool::Execute([this, pJob] {
                        pJob->Function();
                        m_Lock.lock();
                        DeregisterJobExecution(*pJob);
                        m_Lock.unlock();
                    }));
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
                    SetPhase(m_CurrentPhase + 1u);
                }
            } else
            {
                m_ScheduleTimeoutCvar.wait(uLock);
            }
        }
    }

    void JobScheduler::ScheduleJob(const Job& job, uint32_t phase)
    {
        m_Lock.lock();
        phase = phase == CURRENT_PHASE ? m_CurrentPhase : phase;
        m_Jobs[phase].EmplaceBack(job);
        m_JobIndices[phase].PushBack(m_Jobs[phase].GetSize() - 1u);

        m_ScheduleTimeoutCvar.notify_all();
        m_Lock.unlock();
    }

    void JobScheduler::ScheduleJobs(const TArray<Job>& jobs, uint32_t phase)
    {
        m_Lock.lock();
        phase = phase == CURRENT_PHASE ? m_CurrentPhase : phase;

        // Push jobs
        uint32 oldJobsCount = m_Jobs[phase].GetSize();
        m_Jobs[phase].Resize(oldJobsCount + jobs.GetSize());
        std::copy_n(jobs.begin(), jobs.GetSize(), &m_Jobs[phase][oldJobsCount]);

        // Push job indices
        TArray<uint32>& jobIndices = m_JobIndices[phase];
        uint32 oldIndicesCount = m_JobIndices[phase].GetSize();
        m_JobIndices[phase].Resize(oldIndicesCount + jobs.GetSize());
        std::iota(&jobIndices[oldIndicesCount - 1u], &jobIndices.GetBack(), oldJobsCount);

        m_ScheduleTimeoutCvar.notify_all();

        m_Lock.unlock();
    }

    uint32 JobScheduler::ScheduleRegularJob(const Job& job, uint32_t phase)
    {
        m_Lock.lock();

        uint32 jobID = m_RegularJobIDGenerator.GenID();
        m_RegularJobs[phase].PushBack(job, jobID);

        m_Lock.unlock();

        return jobID;
    }

    void JobScheduler::DescheduleRegularJob(uint32 phase, uint32 jobID)
    {
        m_Lock.lock();
        m_RegularJobs[phase].Pop(jobID);
        m_Lock.unlock();
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

        if (m_CurrentPhase < g_PhaseCount)
        {
            for (uint32& jobID : m_RegularJobIDsToTick)
            {
                const Job& job = m_RegularJobs[m_CurrentPhase].IndexID(jobID);
                if (CanExecute(job))
                {
                    jobID = m_RegularJobIDsToTick.GetBack();
                    m_RegularJobIDsToTick.PopBack();

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
            auto processingComponentItr = m_ProcessingComponents.find(componentReg.TID);
            if (processingComponentItr != m_ProcessingComponents.end() && (componentReg.Permissions == RW || processingComponentItr->second == 0))
            {
                return false;
            }
        }

        return true;
    }

    void JobScheduler::RegisterJobExecution(const Job& job)
    {
        for (const ComponentAccess& componentReg : job.Components)
        {
            uint32 isReadOnly = componentReg.Permissions == R;

            auto processingComponentItr = m_ProcessingComponents.find(componentReg.TID);
            if (processingComponentItr == m_ProcessingComponents.end())
            {
                m_ProcessingComponents.insert({componentReg.TID, isReadOnly});
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
            auto processingComponentItr = m_ProcessingComponents.find(componentReg.TID);
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
        if (m_CurrentPhase < g_PhaseCount + 1)
        {
            m_Jobs[m_CurrentPhase].Clear();
        }

        m_CurrentPhase = phase;

        if (m_CurrentPhase < g_PhaseCount)
        {
            m_RegularJobIDsToTick = m_RegularJobs[m_CurrentPhase].GetIDs();
        }
    }

    bool JobScheduler::PhaseJobsExist() const
    {
        return !m_RegularJobIDsToTick.IsEmpty() || !m_JobIndices[m_CurrentPhase].IsEmpty();
    }
}
