// Socket.cpp: implementation of the Connection class.
//
//////////////////////////////////////////////////////////////////////

#include "Socket.h"
#include "Packet.h"

// Socket headers
//#include <sys/types.h>

//#include <unistd.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>

// // #define SIM_LAG 10

long g_nSimBandwidth = 0;
long g_nAvailBandwidth = 0;

long g_nPacketsSent = 0;
long g_nPacketsReceived = 0;
long g_nBytesSent = 0;
long g_nBytesReceived = 0;


//////////////////////////////////////////////////////////////////////
// Operations
//////////////////////////////////////////////////////////////////////

int Socket::Connect( const char* szAddress, int nPort )
{
	Disconnect();

	// Resolve the hostname to IP
	PHOSTENT phe = gethostbyname( szAddress );
	if( phe == NULL ) return E_UNKNOWN_HOST;

    // Build the remote server address
    struct sockaddr_in saAddress;
    saAddress.sin_family = AF_INET;
	memcpy( &saAddress.sin_addr.s_addr, phe->h_addr, phe->h_length );
    saAddress.sin_port = htons( nPort );

	// Create a streaming client socket
    m_fdSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if( m_fdSocket == INVALID_SOCKET ) return E_SOCKET;

	// Connect to the remote server
    if( connect( m_fdSocket, (struct sockaddr*)&saAddress, sizeof(sockaddr_in) ) != 0 )
		return E_CONN_REFUSED;

	// Make the socket non-blocking
	unsigned long nValue = 1;
	if( ioctlsocket( m_fdSocket, FIONBIO, &nValue ) != 0 )
		return E_SOCKET;

	// Disable buffering of send data
	BOOL bOpt = TRUE;
	if( setsockopt( m_fdSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&bOpt, sizeof(BOOL) ) != 0 )
		return E_SOCKET;

	return SOCK_OK;
}

int Socket::Attach( SOCKET fd )
{
	Disconnect();

	if( fd == INVALID_SOCKET )
		return SOCK_OK;

	m_fdSocket = fd;

	// Make the socket non-blocking
	unsigned long nValue = 1;
	if( ioctlsocket( m_fdSocket, FIONBIO, &nValue ) != 0 )
		return E_SOCKET;

	return SOCK_OK;
}

int Socket::UpdateSend()
{
	if( m_fdSocket == INVALID_SOCKET ) return E_SOCKET;

#ifdef SIM_LAG
	static int socket_lag2 = 0;
	if( !socket_lag2 ) socket_lag2 = SIM_LAG;
	else { socket_lag2--; return SOCK_OK; }
#endif

	// Push all packets waiting to be sent
	while( m_lstSend.size() )
	{
		Packet* pSending = m_lstSend.front();

		int nLength = pSending->GetPacketSize() - m_nSentData;
		const char* pData = (const char*)pSending->GetPacketData() + m_nSentData;

		int nSimLength = nLength;
		if( g_nSimBandwidth )
		{
			// simulate bandwidth limit
			if( !g_nAvailBandwidth ) break;
			if( nSimLength > g_nAvailBandwidth )
				nSimLength = g_nAvailBandwidth;
		}

		// Try to send all remaining data
		int nSent = send( m_fdSocket, pData, nSimLength, 0 );
		if( nSent == SOCKET_ERROR )
		{
			int nErr = WSAGetLastError();
			if( nErr != WSAEWOULDBLOCK )
				return E_SOCKET;
			break;
		}

		if( g_nSimBandwidth )
		{
			g_nAvailBandwidth -= nSent;
			if( g_nAvailBandwidth < 0 ) g_nAvailBandwidth = 0;
		}

		if( nSent >= nLength )
		{
			// Finished sending this packet
			g_nBytesSent += pSending->GetPacketSize();
			g_nPacketsSent++;

			pSending->Release();
			m_lstSend.pop_front();
			m_nSentData = 0;
		}
		else
		{
			// Network buffer is full, try again later
			m_nSentData -= nSent;
			break;
		}
	}

	return SOCK_OK;
}

int Socket::UpdateReceive()
{
	if( m_fdSocket == INVALID_SOCKET ) return E_SOCKET;

#ifdef SIM_LAG
	static int socket_lag = 0;
	if( !socket_lag ) socket_lag = SIM_LAG;
	else { socket_lag--; return SOCK_OK; }
#endif

	// Shove all packets waiting to be received
	for(;;)
	{
		if( !m_bGotHeader )
		{
			if( !m_pReceiving ) m_pReceiving = new Packet;

			int nExpect = m_pReceiving->GetHeaderSize() - m_nReceivedData;
			byte* pBuffer = m_pReceiving->GetPacketData() + m_nReceivedData;

			int nSimExpect = nExpect;
			if( g_nSimBandwidth )
			{
				// simulate bandwidth limit
				if( !g_nAvailBandwidth )
					return SOCK_OK; // no data ready
				if( nSimExpect > g_nAvailBandwidth )
					nSimExpect = g_nAvailBandwidth;
			}

			long nReady = recv( m_fdSocket, (char*)pBuffer, nSimExpect, 0 );
			if( nReady == SOCKET_ERROR )
			{
				int nErr = WSAGetLastError();
				if( nErr == WSAEWOULDBLOCK )
					return SOCK_OK; // no data ready
				return nErr;
			}
			if( nReady == 0 ) return E_CONN_CLOSED;

			if( g_nSimBandwidth )
			{
				g_nAvailBandwidth -= nReady;
				if( g_nAvailBandwidth < 0 ) g_nAvailBandwidth = 0;
			}

			if( nReady >= nExpect )
			{
				// Finished receiving the header
				g_nBytesReceived += m_pReceiving->GetHeaderSize();

				m_bGotHeader = true;
				m_nReceivedData = 0;

				m_pReceiving->UnpackHeader();
			}
			else
			{
				// Wait for the rest of the data
				m_nReceivedData += nReady;
				return SOCK_OK; // wait for more
			}
		}

		if( m_bGotHeader )
		{
			int nExpect = m_pReceiving->GetLength() - m_nReceivedData;
			byte* pBuffer = m_pReceiving->GetData() + m_nReceivedData;

			int nSimExpect = nExpect;
			if( g_nSimBandwidth )
			{
				// simulate bandwidth limit
				if( !g_nAvailBandwidth ) return SOCK_OK; // no data ready
				if( nSimExpect > g_nAvailBandwidth )
					nSimExpect = g_nAvailBandwidth;
			}

			long nReady = recv( m_fdSocket, (char*)pBuffer, nSimExpect, 0 );
			if( nReady == SOCKET_ERROR )
			{
				int nErr = WSAGetLastError();
				if( nErr == WSAEWOULDBLOCK )
					return SOCK_OK; // no data ready
				return nErr;
			}
			if( nReady == 0 ) return E_CONN_CLOSED;

			if( g_nSimBandwidth )
			{
				g_nAvailBandwidth -= nReady;
				if( g_nAvailBandwidth < 0 ) g_nAvailBandwidth = 0;
			}

			if( nReady >= nExpect )
			{
				// Finished receiving this packet
				g_nBytesReceived += m_pReceiving->GetLength();
				g_nPacketsReceived++;

				m_lstReceive.push_back( m_pReceiving );
				m_pReceiving = NULL;
				m_nReceivedData = 0;
				m_bGotHeader = false;
			}
			else
			{
				// Wait for the rest of the data
				m_nReceivedData += nReady;
				return SOCK_OK; // wait for more
			}
		}
	}

	return SOCK_OK;
}

void Socket::SendPacket( Packet* pPacket )
{
	m_lstSend.push_back( pPacket );
	pPacket->AddRef();
}

Packet* Socket::ReceivePacket()
{
	if( !m_lstReceive.size() ) return NULL;
	Packet* pGot = m_lstReceive.front();
	m_lstReceive.pop_front();
	return pGot;
}
