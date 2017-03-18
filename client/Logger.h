// Logger.h: log file writer
//
//////////////////////////////////////////////////////////////////////

#ifndef LOGGER_H
#define LOGGER_H

// Initialise the logger
//
int log_Open( const char *szFilename, void* hWnd );
int log_Close();

// Log an error
//
int log_Log( const char *msg );
int log_Logf( const char *fmt, ... );

#endif // LOGGER_H
