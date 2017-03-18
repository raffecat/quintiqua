// Logger.cpp: log file writer
//
//////////////////////////////////////////////////////////////////////

#include "global.h"

#ifdef WINDOWS
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <algorithm>
#endif

#include "Logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include <stdarg.h>

#define LOG_BUFFER 1024


static FILE* m_file = NULL;

#ifdef WINDOWS
static HWND m_hWndLog = NULL;
#endif

int log_Open( const char *szFilename, void* hWnd )
{
	static char szTimeString[30];
	time_t tmNow;

	if( m_file ) log_Close();

#ifdef WINDOWS
	// Keep the window handle
	m_hWndLog = (HWND) hWnd;
	SendMessage( m_hWndLog, EM_LIMITTEXT, 0, 0 ); // maximum limit.
#endif

	// Open the file for append
	m_file = fopen( szFilename, "wt" );
	if( !m_file ) return ferror(m_file);

	// Log the current time
	tmNow = time(NULL);
	strcpy( szTimeString, ctime( &tmNow ) );
	szTimeString[24] = '\0';
	log_Logf( "-------- Log opened %s", szTimeString );

	return 0;
}

int log_Close()
{
	if( m_file )
	{
		static char szTimeString[30];
		time_t tmNow;

		// Log the current time
		tmNow = time(NULL);
		strcpy( szTimeString, ctime( &tmNow ) );
		szTimeString[24] = '\0';
		log_Logf( "-------- Log closed %s", szTimeString );

		// Close the file
		fclose( m_file );
		m_file = NULL;
	}

	return 0;
}

#ifdef WINDOWS
void fixNewlines(std::string& str)
{
	std::string::size_type pos = 0;
	while ( (pos = str.find("\n", pos)) != std::string::npos ) {
		str.replace( pos, 1, "\r\n" );
		pos += 2; // len of replacement text
	}
}

void writeToLogWindow(const char* buf)
{
	std::string str(buf);
	str.append("\n");
	fixNewlines(str);

	//SetWindowRedraw( m_hWndLog, FALSE );
	LRESULT nLen = SendMessage( m_hWndLog, WM_GETTEXTLENGTH, 0, 0 );
	SendMessage( m_hWndLog, EM_SETSEL, (WPARAM)nLen, (LPARAM)nLen );
	SendMessage( m_hWndLog, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)str.c_str() );
	//SetWindowRedraw( m_hWndLog, TRUE );
	//InvalidateRect( m_hWndLog, NULL, FALSE );
}
#endif

int log_Logf( const char *fmt, ... )
{
	va_list vaArgs;
	static char szBuffer[LOG_BUFFER + 1];

	// Build a line from the arguments
	va_start( vaArgs, fmt );
	vsprintf( szBuffer, fmt, vaArgs );
	va_end( vaArgs );

	// Write the line to the log file
	fprintf( m_file, "%s\n", szBuffer );

#ifdef WINDOWS
#ifdef _DEBUG
	// Write the line to the debugger
	//OutputDebugString( szLine );
#endif
	// Add the line to the log window
	if( m_hWndLog ) writeToLogWindow(szBuffer);
#else
	// Write to the console
	printf( "%s\n", szBuffer );
#endif

	return 0;
}


int log_Log( const char *msg )
{
	// Write the line to the log file
	fprintf( m_file, "%s\n", msg );

	// Write the line to the debugger
#ifdef _DEBUG
//	OutputDebugString( msg );
//	OutputDebugString( "\n" );
#endif

#ifdef WINDOWS
	// Add the line to the log window
	if( m_hWndLog ) writeToLogWindow(msg);
#else
	// Write to the console
	printf( "%s\n", msg );
#endif

	return 0;
}
