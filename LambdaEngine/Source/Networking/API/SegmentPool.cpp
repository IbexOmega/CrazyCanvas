#include "Networking/API/SegmentPool.h"
#include "Networking/API/NetworkSegment.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SegmentPool::SegmentPool(uint16 size)
	{
		m_Segments.Reserve(size);
		m_SegmentsFree.Reserve(size);

		for (uint16 i = 0; i < size; i++)
		{
			NetworkSegment* pSegment = DBG_NEW NetworkSegment();
			m_Segments.PushBack(pSegment);
			m_SegmentsFree.PushBack(pSegment);
		}		
	}

	SegmentPool::~SegmentPool()
	{
		for (uint16 i = 0; i < m_Segments.GetSize(); i++)
			delete m_Segments[i];

		m_Segments.Clear();
	}

	NetworkSegment* SegmentPool::RequestFreeSegment()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		NetworkSegment* pSegment = nullptr;
		if (!m_SegmentsFree.IsEmpty())
		{
			pSegment = m_SegmentsFree[m_SegmentsFree.GetSize() - 1];
			m_SegmentsFree.PopBack();

#ifndef LAMBDA_CONFIG_PRODUCTION
			Request(pSegment);
#endif
		}
		else
		{
			LOG_ERROR("[SegmentPool]: No more free packets!, delta = -1");
		}
		return pSegment;
	}

	bool SegmentPool::RequestFreeSegments(uint16 nrOfSegments, TArray<NetworkSegment*>& segmentsReturned)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		int32 delta = (int32)m_SegmentsFree.GetSize() - nrOfSegments;

		if (delta < 0)
		{
			LOG_ERROR("[SegmentPool]: No more free segments!, delta = %d", delta);
			return false;
		}

		segmentsReturned = TArray<NetworkSegment*>(m_SegmentsFree.begin() + delta, m_SegmentsFree.end());
		m_SegmentsFree = TArray<NetworkSegment*>(m_SegmentsFree.begin(), m_SegmentsFree.begin() + delta);

#ifndef LAMBDA_CONFIG_PRODUCTION
		for (int32 i = 0; i < nrOfSegments; i++)
		{
			Request(segmentsReturned[i]);
		}
#endif

		return true;
	}

	void SegmentPool::FreeSegment(NetworkSegment* pSegment)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		Free(pSegment);
	}

	void SegmentPool::FreeSegments(TArray<NetworkSegment*>& segments)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		for (NetworkSegment* pSegment : segments)
		{
			Free(pSegment);
		}
		segments.Clear();
	}

	void SegmentPool::Request(NetworkSegment* pSegment)
	{
		pSegment->m_IsBorrowed = true;

#ifdef DEBUG_PACKET_POOL
		LOG_MESSAGE("[SegmentPool]: Lending [%x]", pSegment);
#endif
	}

	void SegmentPool::Free(NetworkSegment* pSegment)
	{
#ifdef DEBUG_PACKET_POOL
		LOG_MESSAGE("[SegmentPool]: Freeing [%x]%s", pSegment, pSegment->ToString().c_str());
#endif

#ifndef LAMBDA_CONFIG_PRODUCTION
		if (pSegment->m_IsBorrowed)
		{
			pSegment->m_IsBorrowed = false;
		}
		else
		{
			LOG_ERROR("[SegmentPool]: Packet was returned multiple times!");
			DEBUGBREAK();
		}
#endif

		pSegment->m_SizeOfBuffer = 0;
		m_SegmentsFree.PushBack(pSegment);
	}

	void SegmentPool::Reset()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_SegmentsFree.Clear();
		m_SegmentsFree.Reserve(m_Segments.GetSize());

		for (NetworkSegment* pSegment : m_Segments)
		{
#ifndef LAMBDA_CONFIG_PRODUCTION
			pSegment->m_IsBorrowed = false;
#endif
			pSegment->m_SizeOfBuffer = 0;
			m_SegmentsFree.PushBack(pSegment);
		}
	}

	uint16 SegmentPool::GetSize() const
	{
		return (uint16)m_Segments.GetSize();
	}

	uint16 SegmentPool::GetFreeSegments() const
	{
		return (uint16)m_SegmentsFree.GetSize();
	}
}