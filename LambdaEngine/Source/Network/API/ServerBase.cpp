#include "Network/API/ServerBase.h"
#include "Threading/Thread.h"
#include "Log/Log.h"

namespace LambdaEngine
{
	ServerBase::ServerBase() : 
		m_pThread(nullptr),
		m_Run(true),
		m_ReadyForStart(false),
		m_Release(false),
		m_Port(0)
	{

	}

	ServerBase::~ServerBase()
	{
		if (!m_Release)
			LOG_ERROR("[ServerBase]: Do not use delete on a Server. Use the Release() function!");
	}

	bool ServerBase::Start(const std::string& address, uint16 port)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (ThreadHaveTerminated())
		{
			m_Address = address;
			m_Port = port;
			return StartThread();
		}
		return false;
	}

	void ServerBase::Stop()
	{
		OnStopRequested();
		TerminateThread();
	}

	void ServerBase::Release()
	{
		if (!m_Release)
		{
			m_Release = true;
			Stop();
			OnReleaseRequested();
			TerminateThread();
		}

		if (ThreadHaveTerminated())
		{
			delete this;
		}
	}

	const std::string& ServerBase::GetAddress()
	{
		return m_Address;
	}

	uint16 ServerBase::GetPort()
	{
		return m_Port;
	}

	/*******************************************
	*				PROTECTED				   *
	********************************************/

	bool ServerBase::StartThread()
	{
		if (ThreadHaveTerminated())
		{
			m_Run = true;
			m_ReadyForStart = false;
			m_Release = false;

			m_pThread = Thread::Create(
				std::bind(&ServerBase::ThreadServer, this),
				std::bind(&ServerBase::ThreadServerDeleted, this)
			);

			m_ReadyForStart = true;
			return true;
		}

		return false;
	}

	void ServerBase::TerminateThread()
	{
		m_Run = false;
	}

	bool ServerBase::ShouldTerminate() const
	{
		return !m_Run;
	}

	bool ServerBase::ThreadHaveTerminated() const
	{
		return m_pThread == nullptr;
	}

	void ServerBase::SetAddressAndPort(const std::string& address, uint16 port)
	{
		m_Address = address;
		m_Port = port;
	}

	/*******************************************
	*					PRIVATE				   *
	********************************************/

	void ServerBase::ThreadServer()
	{
		while (!m_ReadyForStart) {}

		OnThreadStarted();

		while (!ShouldTerminate())
		{
			OnThreadUpdate();
		}
	}

	/*
	* Called right before delete m_pThread
	*/
	void ServerBase::ThreadServerDeleted()
	{
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			OnThreadTerminated();
			m_pThread = nullptr;
		}

		if (m_Release)
		{
			Release();
		}
		LOG_WARNING("Server Thread Terminated");
	}
}