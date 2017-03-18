// Packet.cpp: implementation of the Packet class.
//
//////////////////////////////////////////////////////////////////////

#include "Packet.h"
#include "Socket.h"

#include <assert.h>


extern HACCEL m_hAccelTable;

void Packet::UnpackHeader()
{
	int len = UNPACK_UINT16(m_pData, 0);
	m_pData[len] = 0; // terminate packet.
}

bool Packet::CloseMessage()
{
	assert( PACKET_DATA_LENGTH <= MAX_PACKET_DATA );

	int len = PACKET_DATA_LENGTH;
	m_pData[0] = (unsigned char) (len >> 8);
	m_pData[1] = (unsigned char) (len);

	return (len > 0); // packet contains some data?
}
