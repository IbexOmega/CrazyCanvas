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

#ifdef LAMBDA_CONFIG_DEBUG
	NetworkSegment* SegmentPool::RequestFreeSegment(const std::string& borrower)
	{

		if (borrower.empty())
		{
			DEBUGBREAK();
		}

		NetworkSegment* pSegment = RequestFreeSegment();
		pSegment->m_Borrower = borrower;
		pSegment->m_IsBorrowed = true;
		return pSegment;
		return nullptr;
	}

	bool SegmentPool::RequestFreeSegments(uint16 nrOfSegments, TArray<NetworkSegment*>& segmentsReturned, const std::string& borrower)
	{
		if (borrower.empty())
		{
			DEBUGBREAK();
		}

		if (RequestFreeSegments(nrOfSegments, segmentsReturned))
		{
			for (NetworkSegment* pSegment : segmentsReturned)
			{
				pSegment->m_Borrower = borrower;
				pSegment->m_IsBorrowed = true;
			}
			return true;
		}
		return false;
	}
#endif

	NetworkSegment* SegmentPool::RequestFreeSegment()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		NetworkSegment* pSegment = nullptr;
		if (!m_SegmentsFree.IsEmpty())
		{
			pSegment = m_SegmentsFree[m_SegmentsFree.GetSize() - 1];
			m_SegmentsFree.PopBack();
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

	void SegmentPool::Free(NetworkSegment* pSegment)
	{
#ifdef LAMBDA_CONFIG_DEBUG
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
#ifdef LAMBDA_CONFIG_DEBUG
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