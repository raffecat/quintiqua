// Socket.h: FGM Socket encapsulation class
//
//////////////////////////////////////////////////////////////////////

#ifndef FGM_SOCKET
#define FGM_SOCKET

#ifdef WINDOWS
#include <winsock.h>
#else
#include <socket.h>
#endif

#include <deque>

enum SocketError {
	SOCK_OK = 0,
	E_SOCKET = 1,
	E_UNKNOWN_HOST = 2,
	E_CONN_REFUSED = 3,
	E_CONN_CLOSED = 4,
	E_PORT_IN_USE = 5,
	E_SOCKET_ERROR = 6,
};

class Packet;

class Socket
{
public:
	// Constructor
	//
	Socket() : m_fdSocket(INVALID_SOCKET), m_pReceiving(NULL),
		m_nSentData(0), m_nReceivedData(0), m_bGotHeader(false)
	{
	}

	// Destructor
	//
	~Socket()
	{
		Disconnect();
	}

	// Connection management
	//
	int Connect( const char* szAddress, int nPort );
	int Attach( SOCKET fd );

	void Disconnect()
	{
		if( m_fdSocket != INVALID_SOCKET ) closesocket( m_fdSocket );
		m_fdSocket = INVALID_SOCKET;
	}

	bool IsConnected()
	{
		return (m_fdSocket != INVALID_SOCKET);
	}

	int UpdateSend();
	int UpdateReceive();

	// Packet sending methods
	//
	void SendPacket( Packet* pPacket );
	Packet* ReceivePacket();

public:
	typedef std::deque<Packet*> PacketQueue;

	// Allow server classes to peek
	SOCKET m_fdSocket;
	PacketQueue m_lstSend;
	PacketQueue m_lstReceive;
	Packet* m_pReceiving;
	int m_nSentData;
	int m_nReceivedData;
	bool m_bGotHeader;
};

#endif // FGM_SOCKET
