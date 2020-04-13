#pragma once
#include "../API/ISocket.h"
#include "Types.h"
#include "Log/Log.h"

#include "Containers/String.h"
#include "Containers/THashTable.h"

#include "Networking/API/IPEndPoint.h"
#include "Networking/API/IPAddress.h"

#ifdef LAMBDA_PLATFORM_WINDOWS

#include <winsock2.h>
#include <Ws2tcpip.h>

#include "Networking/Win32/Win32IPAddress.h"

namespace LambdaEngine
{
	template <typename IBase>
	class Win32SocketBase : public IBase
	{	
		friend class Win32SocketFactory;

	public:

		/*
		* Binds the socket to a given ip-address and port. To bind a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* pIPEndPoint - The IPEndPoint to bind the socket to
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Bind(const IPEndPoint& pIPEndPoint) override
		{
			struct sockaddr_in socketAddress;
			IPEndPointToSocketAddress(&pIPEndPoint, &socketAddress);

			if (bind(m_Socket, (struct sockaddr*) &socketAddress, sizeof(sockaddr_in)) == SOCKET_ERROR)
			{
				LOG_ERROR_CRIT("Failed to bind to %s", pIPEndPoint.ToString().c_str());
				PrintLastError();
				return false;
			}
			m_pIPEndPoint = pIPEndPoint;

			ReadSocketData();

			return true;
		};

		/*
		* Connects the socket to a given ip-address and port. To connect to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* pIPEndPoint - The IPEndPoint to connect the socket to
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Connect(const IPEndPoint& pIPEndPoint) override
		{
			struct sockaddr_in socketAddress;
			IPEndPointToSocketAddress(&pIPEndPoint, &socketAddress);

			if (connect(m_Socket, reinterpret_cast<sockaddr*>(&socketAddress), sizeof(socketAddress)) == SOCKET_ERROR)
			{
				LOG_ERROR_CRIT("Failed to connect to %s", pIPEndPoint.ToString().c_str());
				PrintLastError();
				return false;
			}
			m_pIPEndPoint = pIPEndPoint;

			ReadSocketData();

			return true;
		};

		/*
		* Sets the socket in non blocking or blocking mode.
		*
		* enable - True to use blocking calls, false for non blocking calls.
		*
		* return - False if an error occured, otherwise true.
		*/
		virtual bool EnableBlocking(bool enable) override
		{
			if (ioctlsocket(m_Socket, FIONBIO, &((u_long)enable)) != NO_ERROR)
			{
				LOG_ERROR_CRIT("Failed to change blocking mode to [%sBlocking] ", enable ? "Non " : "");
				PrintLastError();
				return false;
			}
			
			m_NonBlocking = enable;
			return true;
		};

		virtual bool IsNonBlocking() const override
		{
			return m_NonBlocking;
		};

		/*
		* Closes the socket
		*
		* return - False if an error occured, otherwise true.
		*/
		virtual bool Close() override
		{
			if (m_Closed)
				return true;

			m_Closed = true;

			if (closesocket(m_Socket) == SOCKET_ERROR)
			{
				LOG_ERROR_CRIT("Failed to close socket");
				PrintLastError();
				return false;
			}
			return true;
		};

		virtual bool IsClosed() const override
		{
			return m_Closed;
		};

		/*
		* return - The IPEndPoint currently Bound or Connected to
		*/
		virtual const IPEndPoint& GetEndPoint() const override
		{
			return m_pIPEndPoint;
		}

	protected:
		Win32SocketBase() : Win32SocketBase(INVALID_SOCKET, IPEndPoint(IPAddress::ANY, 0))
		{

		};

		Win32SocketBase(uint64 socket, const IPEndPoint& pIPEndPoint) :
			m_Socket(socket),
			m_NonBlocking(false),
			m_Closed(false),
			m_pIPEndPoint(pIPEndPoint)
		{

		};

		~Win32SocketBase()
		{
			Close();
		};

		void ReadSocketData()
		{
			sockaddr_in socketAddress;
			socklen_t socketAddressSize = sizeof(socketAddress);
			if (getsockname(m_Socket, reinterpret_cast<sockaddr*>(&socketAddress), &socketAddressSize) == SOCKET_ERROR)
			{
				LOG_ERROR_CRIT("Faild to ReadSocketData");
				return;
			}

			inet_ntop(socketAddress.sin_family, &socketAddress.sin_addr, m_pReceiveAddressBuffer, s_ReceiveAddressBufferSize);
			uint16 port = ntohs(socketAddress.sin_port);

			m_pIPEndPoint.SetEndPoint(IPAddress::Get(m_pReceiveAddressBuffer), port);
		}

	protected:
		static void IPEndPointToSocketAddress(const IPEndPoint* pIPEndPoint, struct sockaddr_in* socketAddress)
		{
			socketAddress->sin_family = AF_INET;
			socketAddress->sin_port = htons(pIPEndPoint->GetPort());
			socketAddress->sin_addr = *((Win32IPAddress*)pIPEndPoint->GetAddress())->GetWin32Addr();
		}

		static void PrintLastError()
		{
			std::string message;
			int32 errorCode = WSAGetLastError();

			switch (errorCode)
			{
			case WSA_INVALID_HANDLE: message = "Specified event object handle is invalid. An application attempts to use an event object, but the specified handle is not valid."; break;
			case WSA_NOT_ENOUGH_MEMORY: message = "Insufficient memory available. An application used a Windows Sockets function that directly maps to a Windows function.The Windows function is indicating a lack of required memory resources."; break;
			case WSA_INVALID_PARAMETER: message = "One or more parameters are invalid. An application used a Windows Sockets function which directly maps to a Windows function.The Windows function is indicating a problem with one or more parameters."; break;
			case WSA_OPERATION_ABORTED: message = "Overlapped operation aborted. An overlapped operation was canceled due to the closure of the socket, or the execution of the SIO_FLUSH command in WSAIoctl."; break;
			case WSA_IO_INCOMPLETE: message = "Overlapped I/O event object not in signaled state. The application has tried to determine the status of an overlapped operation which is not yet completed.Applications that use WSAGetOverlappedResult(with the fWait flag set to FALSE) in a polling mode to determine when an overlapped operation has completed, get this error code until the operation is complete."; break;
			case WSA_IO_PENDING: message = "Overlapped operations will complete later. The application has initiated an overlapped operation that cannot be completed immediately.A completion indication will be given later when the operation has been completed."; break;
			case WSAEINTR: message = "Interrupted function call. A blocking operation was interrupted by a call to WSACancelBlockingCall."; break;
			case WSAEBADF: message = "File handle is not valid. The file handle supplied is not valid."; break;
			case WSAEACCES: message = "Permission denied. An attempt was made to access a socket in a way forbidden by its access permissions.An example is using a broadcast address for sendto without broadcast permission being set using setsockopt(SO_BROADCAST). Another possible reason for the WSAEACCES error is that when the bind function is called(on Windows NT 4.0 with SP4 and later), another application, service, or kernel mode driver is bound to the same address with exclusive access.Such exclusive access is a new feature of Windows NT 4.0 with SP4 and later, and is implemented by using the SO_EXCLUSIVEADDRUSE option."; break;
			case WSAEFAULT: message = "Bad address. The system detected an invalid pointer address in attempting to use a pointer argument of a call.This error occurs if an application passes an invalid pointer value, or if the length of the buffer is too small.For instance, if the length of an argument, which is a sockaddr structure, is smaller than the sizeof(sockaddr)."; break;
			case WSAEINVAL: message = "Invalid argument. Some invalid argument was supplied(for example, specifying an invalid level to the setsockopt function).In some instances, it also refers to the current state of the socket�for instance, calling accept on a socket that is not listening."; break;
			case WSAEMFILE: message = "Too many open files. Too many open sockets.Each implementation may have a maximum number of socket handles available, either globally, per process, or per thread."; break;
			case WSAEWOULDBLOCK: message = "Resource temporarily unavailable. This error is returned from operations on nonblocking sockets that cannot be completed immediately, for example recv when no data is queued to be read from the socket.It is a nonfatal error, and the operation should be retried later.It is normal for WSAEWOULDBLOCK to be reported as the result from calling connect on a nonblocking SOCK_STREAM socket, since some time must elapse for the connection to be established."; break;
			case WSAEINPROGRESS: message = "Operation now in progress. A blocking operation is currently executing.Windows Sockets only allows a single blocking operation�per - task or thread�to be outstanding, and if any other function call is made(whether or not it references that or any other socket) the function fails with the WSAEINPROGRESS error."; break;
			case WSAEALREADY: message = "Operation already in progress. An operation was attempted on a nonblocking socket with an operation already in progress�that is, calling connect a second time on a nonblocking socket that is already connecting, or canceling an asynchronous request(WSAAsyncGetXbyY) that has already been canceled or completed."; break;
			case WSAENOTSOCK: message = "Socket operation on nonsocket. An operation was attempted on something that is not a socket.Either the socket handle parameter did not reference a valid socket, or for select, a member of an fd_set was not valid."; break;
			case WSAEDESTADDRREQ: message = "Destination address required. A required address was omitted from an operation on a socket.For example, this error is returned if sendto is called with the remote address of ADDR_ANY."; break;
			case WSAEMSGSIZE: message = "Message too long. A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram was smaller than the datagram itself."; break;
			case WSAEPROTOTYPE: message = "Protocol wrong type for socket. A protocol was specified in the socket function call that does not support the semantics of the socket type requested.For example, the ARPA Internet UDP protocol cannot be specified with a socket type of SOCK_STREAM."; break;
			case WSAENOPROTOOPT: message = "Bad protocol option. An unknown, invalid or unsupported option or level was specified in a getsockopt or setsockopt call."; break;
			case WSAEPROTONOSUPPORT: message = "Protocol not supported. The requested protocol has not been configured into the system, or no implementation for it exists.For example, a socket call requests a SOCK_DGRAM socket, but specifies a stream protocol."; break;
			case WSAESOCKTNOSUPPORT: message = "Socket type not supported. The support for the specified socket type does not exist in this address family.For example, the optional type SOCK_RAW might be selected in a socket call, and the implementation does not support SOCK_RAW sockets at all."; break;
			case WSAEOPNOTSUPP: message = "Operation not supported. The attempted operation is not supported for the type of object referenced.Usually this occurs when a socket descriptor to a socket that cannot support this operation is trying to accept a connection on a datagram socket."; break;
			case WSAEPFNOSUPPORT: message = "Protocol family not supported. The protocol family has not been configured into the system or no implementation for it exists.This message has a slightly different meaning from WSAEAFNOSUPPORT.However, it is interchangeable in most cases, and all Windows Sockets functions that return one of these messages also specify WSAEAFNOSUPPORT."; break;
			case WSAEAFNOSUPPORT: message = "Address family not supported by protocol family. An address incompatible with the requested protocol was used.All sockets are created with an associated address family(that is, AF_INET for Internet Protocols) and a generic protocol type(that is, SOCK_STREAM).This error is returned if an incorrect protocol is explicitly requested in the socket call, or if an address of the wrong family is used for a socket, for example, in sendto."; break;
			case WSAEADDRINUSE: message = "Address already in use. Typically, only one usage of each socket address(protocol / IP address / port) is permitted.This error occurs if an application attempts to bind a socket to an IP address / port that has already been used for an existing socket, or a socket that was not closed properly, or one that is still in the process of closing.For server applications that need to bind multiple sockets to the same port number, consider using setsockopt(SO_REUSEADDR).Client applications usually need not call bind at all�connect chooses an unused port automatically.When bind is called with a wildcard address(involving ADDR_ANY), a WSAEADDRINUSE error could be delayed until the specific address is committed.This could happen with a call to another function later, including connect, listen, WSAConnect, or WSAJoinLeaf."; break;
			case WSAEADDRNOTAVAIL: message = "Cannot assign requested address. The requested address is not valid in its context.This normally results from an attempt to bind to an address that is not valid for the local computer.This can also result from connect, sendto, WSAConnect, WSAJoinLeaf, or WSASendTo when the remote address or port is not valid for a remote computer(for example, address or port 0)."; break;
			case WSAENETDOWN: message = "Network is down. A socket operation encountered a dead network.This could indicate a serious failure of the network system(that is, the protocol stack that the Windows Sockets DLL runs over), the network interface, or the local network itself."; break;
			case WSAENETUNREACH: message = "Network is unreachable. A socket operation was attempted to an unreachable network.This usually means the local software knows no route to reach the remote host."; break;
			case WSAENETRESET: message = "Network dropped connection on reset. The connection has been broken due to keep - alive activity detecting a failure while the operation was in progress.It can also be returned by setsockopt if an attempt is made to set SO_KEEPALIVE on a connection that has already failed."; break;
			case WSAECONNABORTED: message = "Software caused connection abort.An established connection was aborted by the software in your host computer, possibly due to a data transmission time - out or protocol error."; break;
			case WSAECONNRESET: message = "Connection reset by peer. An existing connection was forcibly closed by the remote host.This normally results if the peer application on the remote host is suddenly stopped, the host is rebooted, the host or remote network interface is disabled, or the remote host uses a hard close(see setsockopt for more information on the SO_LINGER option on the remote socket).This error may also result if a connection was broken due to keep - alive activity detecting a failure while one or more operations are in progress.Operations that were in progress fail with WSAENETRESET.Subsequent operations fail with WSAECONNRESET."; break;
			case WSAENOBUFS: message = "No buffer space available. An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full."; break;
			case WSAEISCONN: message = "Socket is already connected. A connect request was made on an already - connected socket.Some implementations also return this error if sendto is called on a connected SOCK_DGRAM socket(for SOCK_STREAM sockets, the to parameter in sendto is ignored) although other implementations treat this as a legal occurrence."; break;
			case WSAENOTCONN: message = "Socket is not connected. A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using sendto) no address was supplied.Any other type of operation might also return this error�for example, setsockopt setting SO_KEEPALIVE if the connection has been reset."; break;
			case WSAESHUTDOWN: message = "Cannot send after socket shutdown. A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.By calling shutdown a partial close of a socket is requested, which is a signal that sending or receiving, or both have been discontinued."; break;
			case WSAETOOMANYREFS: message = "Too many references. Too many references to some kernel object."; break;
			case WSAETIMEDOUT: message = "Connection timed out. A connection attempt failed because the connected party did not properly respond after a period of time, or the established connection failed because the connected host has failed to respond."; break;
			case WSAECONNREFUSED: message = "Connection refused. No connection could be made because the target computer actively refused it.This usually results from trying to connect to a service that is inactive on the foreign host�that is, one with no server application running."; break;
			case WSAELOOP: message = "Cannot translate name. Cannot translate a name."; break;
			case WSAENAMETOOLONG: message = "Name too long. A name component or a name was too long."; break;
			case WSAEHOSTDOWN: message = "Host is down.A socket operation failed because the destination host is down. A socket operation encountered a dead host.Networking activity on the local host has not been initiated.These conditions are more likely to be indicated by the error WSAETIMEDOUT."; break;
			case WSAEHOSTUNREACH: message = "No route to host. A socket operation was attempted to an unreachable host.See WSAENETUNREACH."; break;
			case WSAENOTEMPTY: message = "Directory not empty. Cannot remove a directory that is not empty."; break;
			case WSAEPROCLIM: message = "Too many processes. A Windows Sockets implementation may have a limit on the number of applications that can use it simultaneously.WSAStartup may fail with this error if the limit has been reached."; break;
			case WSAEUSERS: message = "User quota exceeded. Ran out of user quota."; break;
			case WSAEDQUOT: message = "Disk quota exceeded. Ran out of disk quota."; break;
			case WSAESTALE: message = "Stale file handle reference. The file handle reference is no longer available."; break;
			case WSAEREMOTE: message = "Item is remote. The item is not available locally."; break;
			case WSASYSNOTREADY: message = "Network subsystem is unavailable. This error is returned by WSAStartup if the Windows Sockets implementation cannot function at this time because the underlying system it uses to provide network services is currently unavailable.Users should check : That the appropriate Windows Sockets DLL file is in the current path. That they are not trying to use more than one Windows Sockets implementation simultaneously.If there is more than one Winsock DLL on your system, be sure the first one in the path is appropriate for the network subsystem currently loaded. The Windows Sockets implementation documentation to be sure all necessary components are currently installed and configured correctly."; break;
			case WSAVERNOTSUPPORTED: message = "Winsock.dll version out of range. The current Windows Sockets implementation does not support the Windows Sockets specification version requested by the application.Check that no old Windows Sockets DLL files are being accessed."; break;
			case WSANOTINITIALISED: message = "Successful WSAStartup not yet performed. Either the application has not called WSAStartup or WSAStartup failed.The application may be accessing a socket that the current active task does not own(that is, trying to share a socket between tasks), or WSACleanup has been called too many times."; break;
			case WSAEDISCON: message = "Graceful shutdown in progress. Returned by WSARecv and WSARecvFrom to indicate that the remote party has initiated a graceful shutdown sequence."; break;
			case WSAENOMORE: message = "No more results. No more results can be returned by the WSALookupServiceNext function."; break;
			case WSAECANCELLED: message = "Call has been canceled. A call to the WSALookupServiceEnd function was made while this call was still processing.The call has been canceled."; break;
			case WSAEINVALIDPROCTABLE: message = "Procedure call table is invalid. The service provider procedure call table is invalid.A service provider returned a bogus procedure table to Ws2_32.dll.This is usually caused by one or more of the function pointers being NULL."; break;
			case WSAEINVALIDPROVIDER: message = "Service provider is invalid. The requested service provider is invalid.This error is returned by the WSCGetProviderInfo and WSCGetProviderInfo32 functions if the protocol entry specified could not be found.This error is also returned if the service provider returned a version number other than 2.0."; break;
			case WSAEPROVIDERFAILEDINIT: message = "Service provider failed to initialize. The requested service provider could not be loaded or initialized.This error is returned if either a service provider's DLL could not be loaded (LoadLibrary failed) or the provider's WSPStartup or NSPStartup function failed."; break;
			case WSASYSCALLFAILURE: message = "System call failure. A system call that should never fail has failed.This is a generic error code, returned under various conditions. Returned when a system call that should never fail does fail.For example, if a call to WaitForMultipleEvents fails or one of the registry functions fails trying to manipulate the protocol / namespace catalogs. Returned when a provider does not return SUCCESS and does not provide an extended error code.Can indicate a service provider implementation error."; break;
			case WSASERVICE_NOT_FOUND: message = "Service not found. No such service is known.The service cannot be found in the specified name space."; break;
			case WSATYPE_NOT_FOUND: message = "Class type not found. The specified class was not found."; break;
			case WSA_E_NO_MORE: message = "No more results. No more results can be returned by the WSALookupServiceNext function."; break;
			case WSA_E_CANCELLED: message = "Call was canceled. A call to the WSALookupServiceEnd function was made while this call was still processing.The call has been canceled."; break;
			case WSAEREFUSED: message = "Database query was refused. A database query failed because it was actively refused."; break;
			case WSAHOST_NOT_FOUND: message = "Host not found. No such host is known.The name is not an official host name or alias, or it cannot be found in the database(s) being queried.This error may also be returned for protocol and service queries, and means that the specified name could not be found in the relevant database."; break;
			case WSATRY_AGAIN: message = "Nonauthoritative host not found. This is usually a temporary error during host name resolution and means that the local server did not receive a response from an authoritative server.A retry at some time later may be successful."; break;
			case WSANO_RECOVERY: message = "This is a nonrecoverable error. This indicates that some sort of nonrecoverable error occurred during a database lookup.This may be because the database files(for example, BSD - compatible HOSTS, SERVICES, or PROTOCOLS files) could not be found, or a DNS request was returned by the server with a severe error."; break;
			case WSANO_DATA: message = "Valid name, no data record of requested type. The requested name is valid and was found in the database, but it does not have the correct associated data being resolved for.The usual example for this is a host name - to - address translation attempt(using gethostbyname or WSAAsyncGetHostByName) which uses the DNS(Domain Name Server).An MX record is returned but no A record�indicating the host itself exists, but is not directly reachable."; break;
			case WSA_QOS_RECEIVERS: message = "QoS receivers. At least one QoS reserve has arrived."; break;
			case WSA_QOS_SENDERS: message = "QoS senders. At least one QoS send path has arrived."; break;
			case WSA_QOS_NO_SENDERS: message = "No QoS senders. There are no QoS senders."; break;
			case WSA_QOS_NO_RECEIVERS: message = "QoS no receivers. There are no QoS receivers."; break;
			case WSA_QOS_REQUEST_CONFIRMED: message = "QoS request confirmed. The QoS reserve request has been confirmed."; break;
			case WSA_QOS_ADMISSION_FAILURE: message = "QoS admission error. A QoS error occurred due to lack of resources."; break;
			case WSA_QOS_POLICY_FAILURE: message = "QoS policy failure. The QoS request was rejected because the policy system couldn't allocate the requested resource within the existing policy."; break;
			case WSA_QOS_BAD_STYLE: message = "QoS bad style. An unknown or conflicting QoS style was encountered."; break;
			case WSA_QOS_BAD_OBJECT: message = "QoS bad object. A problem was encountered with some part of the filterspec or the provider - specific buffer in general."; break;
			case WSA_QOS_TRAFFIC_CTRL_ERROR: message = "QoS traffic control error. An error with the underlying traffic control(TC) API as the generic QoS request was converted for local enforcement by the TC API.This could be due to an out of memory error or to an internal QoS provider error."; break;
			case WSA_QOS_GENERIC_ERROR: message = "QoS generic error. A general QoS error."; break;
			case WSA_QOS_ESERVICETYPE: message = "QoS service type error. An invalid or unrecognized service type was found in the QoS flowspec."; break;
			case WSA_QOS_EFLOWSPEC: message = "QoS flowspec error. An invalid or inconsistent flowspec was found in the QOS structure."; break;
			case WSA_QOS_EPROVSPECBUF: message = "Invalid QoS provider buffer. An invalid QoS provider - specific buffer."; break;
			case WSA_QOS_EFILTERSTYLE: message = "Invalid QoS filter style. An invalid QoS filter style was used."; break;
			case WSA_QOS_EFILTERTYPE: message = "Invalid QoS filter type. An invalid QoS filter type was used."; break;
			case WSA_QOS_EFILTERCOUNT: message = "Incorrect QoS filter count. An incorrect number of QoS FILTERSPECs were specified in the FLOWDESCRIPTOR."; break;
			case WSA_QOS_EOBJLENGTH: message = "Invalid QoS object length. An object with an invalid ObjectLength field was specified in the QoS provider - specific buffer."; break;
			case WSA_QOS_EFLOWCOUNT: message = "Incorrect QoS flow count. An incorrect number of flow descriptors was specified in the QoS structure."; break;
			case WSA_QOS_EUNKOWNPSOBJ: message = "Unrecognized QoS object. An unrecognized object was found in the QoS provider - specific buffer."; break;
			case WSA_QOS_EPOLICYOBJ: message = "Invalid QoS policy object. An invalid policy object was found in the QoS provider - specific buffer."; break;
			case WSA_QOS_EFLOWDESC: message = "Invalid QoS flow descriptor. An invalid QoS flow descriptor was found in the flow descriptor list."; break;
			case WSA_QOS_EPSFLOWSPEC: message = "Invalid QoS provider-specific flowspec. An invalid or inconsistent flowspec was found in the QoS provider - specific buffer."; break;
			case WSA_QOS_EPSFILTERSPEC: message = "Invalid QoS provider-specific filterspec. An invalid FILTERSPEC was found in the QoS provider - specific buffer."; break;
			case WSA_QOS_ESDMODEOBJ: message = "Invalid QoS shape discard mode object. An invalid shape discard mode object was found in the QoS provider - specific buffer."; break;
			case WSA_QOS_ESHAPERATEOBJ: message = "Invalid QoS shaping rate object. An invalid shaping rate object was found in the QoS provider - specific buffer."; break;
			case WSA_QOS_RESERVED_PETYPE: message = "Reserved policy QoS element type. A reserved policy element was found in the QoS provider - specific buffer."; break;
			default:
				break;
			}

			LOG_ERROR("ERROR CODE: %d", errorCode);
			LOG_ERROR("ERROR MESSAGE: %s\n", message.c_str());
		};

	protected:
		SOCKET m_Socket;
		static constexpr uint8 s_ReceiveAddressBufferSize = 32;
		char m_pReceiveAddressBuffer[s_ReceiveAddressBufferSize];

	private:
		bool m_NonBlocking;
		bool m_Closed;
		IPEndPoint m_pIPEndPoint;
	};
}

#endif