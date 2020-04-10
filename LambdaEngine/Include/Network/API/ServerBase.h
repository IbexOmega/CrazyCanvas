#pragma once
#include "Defines.h"
#include "Types.h"

#include <atomic>

#include "Containers/String.h"
#include "Containers/TArray.h"

#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	class Thread;

	class LAMBDA_API ServerBase
	{
	public:
		ServerBase();
		virtual ~ServerBase();

		/*
		* Starts the server on the given ip-address and port. To bind a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		bool Start(const std::string& address, uint16 port);

		/*
		* Tells the server to stop
		*/
		void Stop();

		/*
		* Release all the resouces used by the server and will be deleted when each thread has terminated.
		*/
		void Release();

		/*
		* return - The currently used inet address.
		*/
		const std::string& GetAddress();

		/*
		* return - The currently used port.
		*/
		uint16 GetPort();

	protected:
		virtual void OnThreadStarted() = 0;
		virtual void OnThreadUpdate() = 0;
		virtual void OnThreadTerminated() = 0;
		virtual void OnStopRequested() = 0;
		virtual void OnReleaseRequested() = 0;

		bool StartThread();
		void TerminateThread();
		bool ShouldTerminate() const;
		bool ThreadHaveTerminated() const;
		void SetAddressAndPort(const std::string& address, uint16 port);

	private:
		void ThreadServer();
		void ThreadServerDeleted();

	private:
		Thread* m_pThread;
		SpinLock m_Lock;

		std::atomic_bool m_Run;
		std::atomic_bool m_ReadyForStart;
		std::atomic_bool m_Release;

		std::string m_Address;
		uint16 m_Port;
	};
}
