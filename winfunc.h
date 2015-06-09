
#ifndef _SFTP_WINFUNC_H
#define _SFTP_WINFUNC_H
#include <windows.h>

/* Constant seems to be missing in MinGW */
#ifndef WC_ERR_INVALID_CHARS
#define WC_ERR_INVALID_CHARS 0x80 
#endif

#define windows_error(winfunc) windows_error_int(winfunc, __FILE__, __LINE__, FALSE)
#define windows_fatal(winfunc) windows_error_int(winfunc, __FILE__, __LINE__, TRUE)

void windows_error_int(LPTSTR function, char *filename, int line, BOOL abort);
LPWSTR utf8_to_lpwstr(char *string);
char * lpwstr_to_utf8(LPWSTR string);

#endif