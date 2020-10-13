#include "ECS/RegularWorker.h"

#include "Containers/TArray.h"
#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	RegularWorker::~RegularWorker()
	{
		ECSCore* pECS = ECSCore::GetInstance();
		if (pECS)
			pECS->DescheduleRegularJob(m_Phase, m_JobID);
	}

	void RegularWorker::ScheduleRegularWork(const Job& job, uint32 phase)
	{
		m_Phase = phase;
		m_JobID = ECSCore::GetInstance()->ScheduleRegularJob(job, phase);
	}

	TArray<ComponentAccess> RegularWorker::GetUniqueComponentAccesses(const EntitySubscriberRegistration& subscriberRegistration)
	{
		// Eliminate duplicate component types across the system's subscriptions
		THashTable<const ComponentType*, ComponentPermissions> uniqueRegs;

		for (const EntitySubscriptionRegistration& subReq : subscriberRegistration.EntitySubscriptionRegistrations)
		{
			RegularWorker::MapComponentAccesses(subReq.ComponentAccesses, uniqueRegs);
		}

		RegularWorker::MapComponentAccesses(subscriberRegistration.AdditionalAccesses, uniqueRegs);

		// Merge all of the system's subscribed component types into one vector
		TArray<ComponentAccess> componentAccesses;
		componentAccesses.Reserve((uint32)uniqueRegs.size());
		for (auto& uniqueRegsItr : uniqueRegs)
		{
			componentAccesses.PushBack({uniqueRegsItr.second, uniqueRegsItr.first});
		}

		return componentAccesses;
	}

	void RegularWorker::MapComponentAccesses(const TArray<ComponentAccess>& componentAccesses, THashTable<const ComponentType*, ComponentPermissions>& uniqueRegs)
	{
		for (const ComponentAccess& componentUpdateReg : componentAccesses)
		{
			if (componentUpdateReg.Permissions == NDA)
			{
				continue;
			}

			auto uniqueRegsItr = uniqueRegs.find(componentUpdateReg.pTID);
			if (uniqueRegsItr == uniqueRegs.end() || componentUpdateReg.Permissions > uniqueRegsItr->second)
			{
				uniqueRegs.insert(uniqueRegsItr, {componentUpdateReg.pTID, componentUpdateReg.Permissions});
			}
		}
	}
}
