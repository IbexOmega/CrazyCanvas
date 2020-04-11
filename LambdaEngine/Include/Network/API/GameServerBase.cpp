#include "GameServerBase.h"

#include "Threading/Thread.h"
#include "PlatformNetworkUtils.h"

namespace LambdaEngine
{
	std::set<GameServerBase*>* GameServerBase::s_Instances;
	SpinLock* GameServerBase::s_LockInstances;
	int64 GameServerBase::s_TimerTransmit;

	GameServerBase::GameServerBase() : 
		m_pThreadReceiver(nullptr),
		m_pThreadTransmitter(nullptr),
		m_pSocket(nullptr),
		m_Port(0),
		m_Run(false),
		m_ThreadsStarted(false),
		m_SocketCreated(false),
		m_ThreadsTerminated(false),
		m_Release(false)
	{
		std::scoped_lock<SpinLock> lock(*s_LockInstances);
		s_Instances->insert(this);
	}

	GameServerBase::~GameServerBase()
	{
		std::scoped_lock<SpinLock> lock(*s_LockInstances);
		s_Instances->erase(this);

		if (!m_Release)
			LOG_ERROR("[GameServerBase]: Do not use delete on a GameServer object. Use the Release() function!");
		else
			LOG_INFO("[GameServerBase]: Released");
	}

	bool GameServerBase::Start(uint16 port)
	{
		std::scoped_lock<SpinLock> lock(m_LockStart);
		if (!ThreadsAreRunning())
		{
			m_Port = port;
			StartThreads();
		}
		return false;
	}

	void GameServerBase::Stop()
	{
		if (!ShouldTerminate())
		{
			TerminateThreads();
			std::scoped_lock<SpinLock> lock(m_LockSocket);
			if(m_pSocket)
				m_pSocket->Close();
		}
	}

	void GameServerBase::Release()
	{
		std::scoped_lock<SpinLock> lock(m_LockRelease);
		if (!m_Release)
		{
			m_Release = true;
			Stop();
		}

		if (m_ThreadsTerminated)
			delete this;
	}

	bool GameServerBase::IsRunning() const
	{
		return m_ThreadsStarted && !m_ThreadsTerminated;
	}

	void GameServerBase::StartThreads()
	{
		m_Run = true;
		m_ThreadsStarted = false;
		m_SocketCreated = false;
		m_ThreadsTerminated = false;

		m_pThreadTransmitter = Thread::Create(
			std::bind(&GameServerBase::ThreadTransmitter, this),
			std::bind(&GameServerBase::ThreadTransmitterDeleted, this)
		);

		m_pThreadReceiver = Thread::Create(
			std::bind(&GameServerBase::ThreadReceiver, this),
			std::bind(&GameServerBase::ThreadReceiverDeleted, this)
		);
		m_ThreadsStarted = true;
	}

	bool GameServerBase::SetupSocket()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (!m_pSocket)
			return false;

		m_Address = PlatformNetworkUtils::GetLocalAddress();

		if (!m_pSocket->Bind(m_Address, m_Port))
			return false;

		if (!m_pSocket->EnableBroadcast(true))
			return false;

		m_Port = m_pSocket->GetPort();
		LOG_INFO("[GameServerBase] Listening on %s:%i", m_Address.c_str(), m_Port);
		return true;
	}

	void GameServerBase::FlushPackets()
	{
		std::scoped_lock<SpinLock> lock(m_LockThread);
		if(m_pThreadTransmitter)
			m_pThreadTransmitter->Notify();
	}

	void GameServerBase::ThreadTransmitter()
	{
		while (!m_ThreadsStarted);
		if (!SetupSocket())
			TerminateThreads();

		m_SocketCreated = true;

		RunTranmitter();

		while (!m_ReceiverStopped);
	}

	void GameServerBase::ThreadReceiver()
	{
		while (!m_SocketCreated);

		RunReceiver();

		m_pThreadTransmitter->Notify();
		m_ReceiverStopped = true;
	}

	void GameServerBase::ThreadTransmitterDeleted()
	{
		{
			std::scoped_lock<SpinLock> lock(m_LockThread);
			m_pThreadTransmitter = nullptr;
		}

		if (!m_pThreadReceiver)
			ThreadsDeleted();
	}

	void GameServerBase::ThreadReceiverDeleted()
	{
		{
			std::scoped_lock<SpinLock> lock(m_LockThread);
			m_pThreadReceiver = nullptr;
		}

		if (!m_pThreadTransmitter)
			ThreadsDeleted();
	}

	void GameServerBase::ThreadsDeleted()
	{
		OnThreadsTurminated();

		std::scoped_lock<SpinLock> lock(m_LockSocket);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		m_ThreadsTerminated = true;
		LOG_WARNING("[GameServerBase] Stopped");

		if (m_Release)
			Release();
	}

	const std::string& GameServerBase::GetAddress() const
	{
		return m_Address;
	}

	uint16 GameServerBase::GetPort() const
	{
		return m_Port;
	}

	bool GameServerBase::ThreadsAreRunning() const
	{
		return m_pThreadReceiver != nullptr && m_pThreadTransmitter != nullptr;
	}

	void GameServerBase::TerminateThreads()
	{
		LOG_WARNING("[GameServerBase] Stopping...");
		m_Run = false;
	}

	bool GameServerBase::ShouldTerminate() const
	{
		return !m_Run;
	}

	bool GameServerBase::Transmit(const std::string& address, uint16 port, char* buffer, int32 length)
	{
		int32 bytesSent = 0;
		if (!m_pSocket->SendTo(buffer, length, bytesSent, address, port))
		{
			return false;
		}
		return length == bytesSent;
	}

	void GameServerBase::InitStatic()
	{
		s_Instances = DBG_NEW std::set<GameServerBase*>();
		s_LockInstances = DBG_NEW SpinLock();
		s_TimerTransmit = 0;
	}

	void GameServerBase::TickStatic(Timestamp dt)
	{
		s_TimerTransmit += dt.AsNanoSeconds();
		if (s_TimerTransmit >= TRANSMIT_DELAY)
		{
			s_TimerTransmit -= TRANSMIT_DELAY;

			std::scoped_lock<SpinLock> lock(*s_LockInstances);
			for (GameServerBase* server : *s_Instances)
				server->FlushPackets();
		}
	}

	void GameServerBase::ReleaseStatic()
	{
		delete s_Instances;
		delete s_LockInstances;
	}
}