#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Types.h"
#include "Log/Log.h"

#include <string>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1

namespace LambdaEngine
{
	template <typename IBase>
	class MacSocketBase : public IBase
	{	
	public:
		bool Bind(const std::string& address, uint16 port) override
		{
			struct sockaddr_in socketAddress;
			socketAddress.sin_family = AF_INET;

			if (!address.empty())
            {
                inet_pton(AF_INET, address.c_str(), &socketAddress.sin_addr.s_addr);
            }
			else
            {
                socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
            }

			socketAddress.sin_port = htons(port);

            if (bind(m_Socket, (struct sockaddr*)&socketAddress, sizeof(sockaddr_in)) == SOCKET_ERROR)
            {
                int32 error = errno;
				LOG_ERROR_CRIT("Failed to bind to %s:%d", !address.empty() ? address.c_str() : "ANY", port);
				PrintLastError(error);
				return false;
			}
			return true;
		}

		virtual bool SetNonBlocking(bool nonBlocking) override
		{
            u_long tempNonBlocking = (u_long)nonBlocking;
			if (ioctl(m_Socket, FIONBIO, &tempNonBlocking) == SOCKET_ERROR)
			{
                int32 error = errno;
				LOG_ERROR_CRIT("Failed to change blocking mode to [%sBlocking] ", nonBlocking ? "Non " : "");
				PrintLastError(error);
				return false;
			}
            
            m_NonBlocking = nonBlocking;
            return true;
		};

        virtual bool IsNonBlocking() const override
        {
            return m_NonBlocking;
        }
                
		virtual bool Close() override
		{
            if (m_Closed)
            {
                return true;
            }

            m_Closed = true;

            if (close(m_Socket) == SOCKET_ERROR)
            {
                int32 error = errno;
                LOG_ERROR_CRIT("Failed to close socket");
                PrintLastError(error);
                return false;
            }
                
            return true;
		}
                
        virtual bool IsClosed() const override
        {
            return m_Closed;
        }

	protected:
        MacSocketBase(int32 socket = INVALID_SOCKET)
            : m_Socket(socket),
            m_NonBlocking(false)
		{
		}
        
       ~MacSocketBase()
        {
            Close();
        }
        
        static void PrintLastError(int32 errorCode)
        {
            const char* pMessage = "";
            switch (errorCode)
            {
                case E2BIG:           pMessage = "Argument list too long (POSIX.1-2001)."; break;
                case EACCES:          pMessage = "Permission denied (POSIX.1-2001)."; break;
                case EADDRINUSE:      pMessage = "Address already in use (POSIX.1-2001)."; break;
                case EADDRNOTAVAIL:   pMessage = "Address not available (POSIX.1-2001)."; break;
                case EAFNOSUPPORT:    pMessage = "Address family not supported (POSIX.1-2001)."; break;
                case EALREADY:        pMessage = "Connection already in progress (POSIX.1-2001)."; break;
                case EBADF:           pMessage = "Bad file descriptor (POSIX.1-2001)."; break;
                case EBADMSG:         pMessage = "Bad message (POSIX.1-2001)."; break;
                case EBUSY:           pMessage = "Device or resource busy (POSIX.1-2001)."; break;
                case ECANCELED:       pMessage = "Operation canceled (POSIX.1-2001)."; break;
                case ECHILD:          pMessage = "No child processes (POSIX.1-2001)."; break;
                case ECONNABORTED:    pMessage = "Connection aborted (POSIX.1-2001)."; break;
                case ECONNREFUSED:    pMessage = "Connection refused (POSIX.1-2001)."; break;
                case ECONNRESET:      pMessage = "Connection reset (POSIX.1-2001)."; break;
                case EDEADLK:         pMessage = "Resource deadlock avoided (POSIX.1-2001)."; break;
                case EDESTADDRREQ:    pMessage = "Destination address required (POSIX.1-2001)."; break;
                case EDOM:            pMessage = "Mathematics argument out of domain of function (POSIX.1, C99)."; break;
                case EDQUOT:          pMessage = "Disk quota exceeded (POSIX.1-2001)."; break;
                case EEXIST:          pMessage = "File exists (POSIX.1-2001)."; break;
                case EFAULT:          pMessage = "Bad address (POSIX.1-2001)."; break;
                case EFBIG:           pMessage = "File too large (POSIX.1-2001)."; break;
                case EHOSTDOWN:       pMessage = "Host is down."; break;
                case EHOSTUNREACH:    pMessage = "Host is unreachable (POSIX.1-2001)."; break;
                case EIDRM:           pMessage = "Identifier removed (POSIX.1-2001)."; break;
                case EILSEQ:          pMessage = "Invalid or incomplete multibyte or wide character (POSIX.1, C99). The text shown here is the glibc error description; in POSIX.1, this error is described as \"Illegal byte sequence\"."; break;
                case EINPROGRESS:     pMessage = "Operation in progress (POSIX.1-2001)."; break;
                case EINTR:           pMessage = "Interrupted function call (POSIX.1-2001); see signal(7)."; break;
                case EINVAL:          pMessage = "Invalid argument (POSIX.1-2001)."; break;
                case EIO:             pMessage = "Input/output error (POSIX.1-2001)."; break;
                case EISCONN:         pMessage = "Socket is connected (POSIX.1-2001)."; break;
                case EISDIR:          pMessage = "Is a directory (POSIX.1-2001)."; break;
                case ELOOP:           pMessage = "Too many levels of symbolic links (POSIX.1-2001)."; break;
                case EMFILE:          pMessage = "Too many open files (POSIX.1-2001).  Commonly caused by exceeding the RLIMIT_NOFILE resource limit described in getrlimit(2)."; break;
                case EMLINK:          pMessage = "Too many links (POSIX.1-2001)."; break;
                case EMSGSIZE:        pMessage = "Message too long (POSIX.1-2001)."; break;
                case EMULTIHOP:       pMessage = "Multihop attempted (POSIX.1-2001)."; break;
                case ENAMETOOLONG:    pMessage = "Filename too long (POSIX.1-2001)."; break;
                case ENETDOWN:        pMessage = "Network is down (POSIX.1-2001)."; break;
                case ENETRESET:       pMessage = "Connection aborted by network (POSIX.1-2001)."; break;
                case ENETUNREACH:     pMessage = "Network unreachable (POSIX.1-2001)."; break;
                case ENFILE:          pMessage = "Too many open files in system (POSIX.1-2001)."; break;
                case ENOBUFS:         pMessage = "No buffer space available (POSIX.1 (XSI STREAMS option))."; break;
                case ENODATA:         pMessage = "No message is available on the STREAM head read queue (POSIX.1-2001)."; break;
                case ENODEV:          pMessage = "No such device (POSIX.1-2001)."; break;
                case ENOENT:          pMessage = "No such file or directory (POSIX.1-2001). Typically, this error results when a specified path‐name does not exist, or one of the components in the directory prefix of a pathname does not exist, or the specified pathname is a dangling symbolic link."; break;
                case ENOEXEC:         pMessage = "Exec format error (POSIX.1-2001)."; break;
                case ENOLCK:          pMessage = "No locks available (POSIX.1-2001)."; break;
                case ENOLINK:         pMessage = "Link has been severed (POSIX.1-2001)."; break;
                case ENOMEM:          pMessage = "Not enough space/cannot allocate memory (POSIX.1-2001)."; break;
                case ENOMSG:          pMessage = "No message of the desired type (POSIX.1-2001)."; break;
                case ENOPROTOOPT:     pMessage = "Protocol not available (POSIX.1-2001)."; break;
                case ENOSPC:          pMessage = "No space left on device (POSIX.1-2001)."; break;
                case ENOSR:           pMessage = "No STREAM resources (POSIX.1 (XSI STREAMS option))."; break;
                case ENOSTR:          pMessage = "Not a STREAM (POSIX.1 (XSI STREAMS option))."; break;
                case ENOSYS:          pMessage = "Function not implemented (POSIX.1-2001)."; break;
                case ENOTBLK:         pMessage = "Block device required."; break;
                case ENOTCONN:        pMessage = "The socket is not connected (POSIX.1-2001)."; break;
                case ENOTDIR:         pMessage = "Not a directory (POSIX.1-2001)."; break;
                case ENOTEMPTY:       pMessage = "Directory not empty (POSIX.1-2001)."; break;
                case ENOTRECOVERABLE: pMessage = "State not recoverable (POSIX.1-2008)."; break;
                case ENOTSOCK:        pMessage = "Not a socket (POSIX.1-2001)."; break;
                case ENOTSUP:         pMessage = "Operation not supported (POSIX.1-2001)."; break;
                case ENOTTY:          pMessage = "Inappropriate I/O control operation (POSIX.1-2001)."; break;
                case ENXIO:           pMessage = "No such device or address (POSIX.1-2001)."; break;
                case EOPNOTSUPP:      pMessage = "Operation not supported on socket (POSIX.1-2001)."; break;
                case EOVERFLOW:       pMessage = "Value too large to be stored in data type (POSIX.1-2001)."; break;
                case EOWNERDEAD:      pMessage = "Owner died (POSIX.1-2008)."; break;
                case EPERM:           pMessage = "Operation not permitted (POSIX.1-2001)."; break;
                case EPFNOSUPPORT:    pMessage = "Protocol family not supported."; break;
                case EPIPE:           pMessage = "Broken pipe (POSIX.1-2001)."; break;
                case EPROTO:          pMessage = "Protocol error (POSIX.1-2001)."; break;
                case EPROTONOSUPPORT: pMessage = "Protocol not supported (POSIX.1-2001)."; break;
                case EPROTOTYPE:      pMessage = "Protocol wrong type for socket (POSIX.1-2001)."; break;
                case ERANGE:          pMessage = "Result too large (POSIX.1, C99)."; break;
                case EREMOTE:         pMessage = "Object is remote."; break;
                case EROFS:           pMessage = "Read-only filesystem (POSIX.1-2001)."; break;
                case ESHUTDOWN:       pMessage = "Cannot send after transport endpoint shutdown."; break;
                case ESPIPE:          pMessage = "Invalid seek (POSIX.1-2001)."; break;
                case ESOCKTNOSUPPORT: pMessage = "Socket type not supported."; break;
                case ESRCH:           pMessage = "No such process (POSIX.1-2001)."; break;
                case ESTALE:          pMessage = "Stale file handle (POSIX.1-2001). This error can occur for NFS and for other filesystems."; break;
                case ETIME:           pMessage = "Timer expired (POSIX.1 (XSI STREAMS option)). (POSIX.1 says \"STREAM ioctl(2) timeout\".)"; break;
                case ETIMEDOUT:       pMessage = "Connection timed out (POSIX.1-2001)."; break;
                case ETOOMANYREFS:    pMessage = "Too many references: cannot splice."; break;
                case ETXTBSY:         pMessage = "Text file busy (POSIX.1-2001)."; break;
                case EUSERS:          pMessage = "Too many users."; break;
                case EWOULDBLOCK:     pMessage = "Operation would block (POSIX.1-2001)."; break;
                case EXDEV:           pMessage = "Improper link (POSIX.1-2001)."; break;
                default:              pMessage = "Unknown error"; break;
            }
            
            LOG_ERROR("ERROR CODE: %d", errorCode);
            LOG_ERROR("ERROR MESSAGE: %s", pMessage);
        }

	protected:
		int32   m_Socket      = INVALID_SOCKET;
        bool    m_NonBlocking = false;
        bool    m_Closed      = false;
	};
}

#endif
