// Packet.h: FGM Packet wrapper class
//
//////////////////////////////////////////////////////////////////////

#ifndef FGM_PACKET_H
#define FGM_PACKET_H

#include <string.h> // memcpy

#define MAX_PACKET_DATA		65535
#define PACKET_HEADER_SIZE	2

#define PACKET_DATA_LENGTH ((int)(m_pPtr - m_pData) - PACKET_HEADER_SIZE)
#define TOTAL_PACKET_SIZE ((int)(m_pPtr - m_pData))

#define UNPACK_INT16(PTR,OFS) (((int)(PTR)[(OFS)]) << 8) | ((int)(PTR)[(OFS)+1]);
#define UNPACK_UINT16(PTR,OFS) (((int)(PTR)[(OFS)]) << 8) | ((int)(PTR)[(OFS)+1]);

class Socket;

class Packet
{
public:
	// Constructor
	//
	Packet() : m_nRefs(1)
	{
		Clear();
	}

	// Destructor
	//
	virtual ~Packet()
	{
	}

	// Packet building functions
	//
	void Clear();
	bool CloseMessage();

	inline void WriteByte( int data );
	inline void WriteInt16( int data );
	inline void WriteInt32( long data );
	inline void WriteData( unsigned char* data, int len );

	// Packet receiving functions
	//
	void UnpackHeader();

	inline int ReadByte();
	inline int ReadInt16();
	inline long ReadInt32();
	inline unsigned char* GetReadPointer();

	// Data access
	//
	inline const int GetHeaderSize() const { return PACKET_HEADER_SIZE; }
	inline unsigned char* GetPacketData() { return m_pData; }
	inline int GetPacketSize() { return TOTAL_PACKET_SIZE; }
	inline unsigned char* GetData() { return m_pData + PACKET_HEADER_SIZE; }
	inline int GetLength() { return PACKET_DATA_LENGTH; }

	// Reference counting
	//
	inline void AddRef() { m_nRefs++; }
	inline void Release() { if( !--m_nRefs ) delete this; }

protected:
	// Packet data (with 2-byte header prefix)
	unsigned char m_pData[PACKET_HEADER_SIZE+MAX_PACKET_DATA+1];

protected:
	// Private members
	unsigned char* m_pPtr;
	int m_nRefs;
};

// Inline implementation functions
//

inline void Packet::Clear()
{
	m_pPtr = m_pData + PACKET_HEADER_SIZE;
}

inline int Packet::ReadByte()
{
	return *m_pPtr++;
}

inline int Packet::ReadInt16()
{
	int nResult = ((int)(*m_pPtr++)) << 8;
	nResult |= *m_pPtr++;
	return nResult;
}

inline long Packet::ReadInt32()
{
	long nResult = ((long)(*m_pPtr++)) << 24;
	nResult |= ((long)(*m_pPtr++)) << 16;
	nResult |= ((long)(*m_pPtr++)) << 8;
	nResult |= *m_pPtr++;
	return nResult;
}

inline unsigned char* Packet::GetReadPointer()
{
	return m_pPtr;
}

inline void Packet::WriteByte( int nData )
{
	*m_pPtr++ = (unsigned char)nData;
}

inline void Packet::WriteInt16( int nData )
{
	*m_pPtr++ = (unsigned char)(nData >> 8);
	*m_pPtr++ = (unsigned char)(nData & 255);
}

inline void Packet::WriteInt32( long nData )
{
	*m_pPtr++ = (unsigned char)(nData >> 24);
	*m_pPtr++ = (unsigned char)((nData >> 16) & 255);
	*m_pPtr++ = (unsigned char)((nData >> 8) & 255);
	*m_pPtr++ = (unsigned char)(nData & 255);
}

inline void Packet::WriteData( unsigned char* data, int len )
{
	memcpy( m_pPtr, data, len );
	m_pPtr += len;
}

#endif // FGM_PACKET_H
