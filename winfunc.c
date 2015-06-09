#ifdef __WIN32__
#define UNICODE

#include <windows.h>
#include "strsafe.h"
#include "log.h"
#include "winfunc.h"

void windows_error_int(LPWSTR lpszFunction, char *filename, int line, BOOL abort) {
	// Retrieve the system error message for the last-error code

    LPWSTR lpMsgBuf;
    LPWSTR lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

	size_t buflen = (lstrlenW(lpMsgBuf) + lstrlenW(lpszFunction) + strlen(filename) + 128) * sizeof(WCHAR);
    lpDisplayBuf = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, buflen);
	
	//LPWSTR errmsg = utf8_to_lpwstr(lpMsgBuf);
	
	char *msg = lpwstr_to_utf8(lpMsgBuf);
	char *func = lpwstr_to_utf8(lpszFunction);
	
	logit("WinAPI call %s failed at file %s:%d with error %d: %s",  func, filename, line, dw, msg);
	free(msg);
	free(func);
	
   // HRESULT ret = StringCchPrintfW(lpDisplayBuf, 
   //     buflen / sizeof(WCHAR),
   //     L"WinAPI call %s failed at file %s, line %d with error %d: %s", 
    //    lpszFunction, file, line, dw, lpMsgBuf); 
	
	//free(errmsg);
	
	/*
	logit("buflen: %i", buflen);
	
	char *msg = lpwstr_to_utf8(lpDisplayBuf);	
	logit("WinAPI call %s failed at file %s:$d with error %d: %s", msg);
	free(msg);
	
	if ( SUCCEEDED(ret) ) {
		MessageBoxW(NULL, lpDisplayBuf, L"Error", MB_OK); 
	} else if ( ret == STRSAFE_E_INVALID_PARAMETER ) {
		logit("Invalid parameter cchDest while formatting error message");
		MessageBoxW(NULL, L"Invalid parameter cchDest while formatting error message", L"Internal error", MB_OK);
	} else if ( ret == STRSAFE_E_INSUFFICIENT_BUFFER ) {
		logit("Insufficient buffer while formatting error message");
		MessageBoxW(NULL, L"Insufficient buffer while formatting error message", L"Internal error", MB_OK);
	} else {
		logit("Unknown error while formatting error message: %i", ret);
		MessageBoxW(NULL, L"Unknown error while formatting error message", L"Internal error", MB_OK);
	}
*/

	
	if ( abort )
		fatal("WinAPI error was fatal, aborting");


    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
	
}

LPWSTR utf8_to_lpwstr(char *string) {
	if ( string == NULL )
		return NULL;
	
	if ( string[0] == '\0' )
		return L"";
	
	int utf8_size = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, string, -1, NULL, 0);
	
	if ( utf8_size <= 0 ) {
		fatal("Error getting allocation size for string conversion");
		return NULL;
	}
	
	int buflen = utf8_size * sizeof(WCHAR);
	LPWSTR utf8_string = malloc(buflen);
	
	int ret = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, string, -1, utf8_string, buflen);
	
	if ( ret <= 0 ) {
		fatal("Error performing UTF8 to UTF16 conversion");
		return NULL;
	}
	
	return utf8_string;
}

char * lpwstr_to_utf8(LPWSTR string) {
	if ( string == NULL )
		return NULL;
	
	if ( string[0] == L'\0' )
		return "";
	
	int utf8_size = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, string, -1, NULL, 0, NULL, NULL );
	
	if ( utf8_size <= 0 ) {
		fatal("Error getting allocation size for wchar to utf8 conversion");
		return NULL;
	}
	
	char *utf8_string = malloc(utf8_size);
	
	int ret = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, string, -1, utf8_string, utf8_size, NULL, NULL );
	
	if ( ret <= 0 ) {
		fatal("Error performing UTF16 to UTF8 conversion");
		return NULL;
	}
	
	return utf8_string;
}


#endif
