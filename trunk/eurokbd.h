#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <stdio.h>

//#include <time.h>
#if 0
struct tm {
        int tm_sec;     /* seconds after the minute - [0,59] */
        int tm_min;     /* minutes after the hour - [0,59] */
        int tm_hour;    /* hours since midnight - [0,23] */
        int tm_mday;    /* day of the month - [1,31] */
        int tm_mon;     /* months since January - [0,11] */
        int tm_year;    /* years since 1900 */
        int tm_wday;    /* days since Sunday - [0,6] */
        int tm_yday;    /* days since January 1 - [0,365] */
        int tm_isdst;   /* daylight savings time flag */
        };
typedef __int64 __time64_t;     /* 64-bit time value */
typedef int errno_t;
//_CRTIMP __cdecl
extern "C" size_t wcsftime(wchar_t *, size_t, const wchar_t *, const struct tm *);
extern "C" errno_t _localtime64_s(struct tm *ptm, const __time64_t *);
#endif

//extern "C" struct tm * _localtime64(const __int64 * _Time);
//_CRTIMP

#include "memcheck.h"

extern LONG g_dwObjectCount;
#define G_OBJECTCOUNT g_dwObjectCount
#include "ptr.h"


#ifdef _WIN32_WCE
#include <Aygshell.h>
#include <keybd.h>
extern "C" LPWSTR  PathCombine(LPWSTR pszDest, LPCWSTR pszDir, LPCWSTR pszFile);

#else
#include <shlwapi.h>

//#include <keybd.h>
#ifndef KeyShiftNoCharacterFlag
#define KeyStatePrevDownFlag        0x0040  //  Key was previously down.
#define KeyStateDownFlag            0x0080  //  Key is currently down.
#define KeyShiftNoCharacterFlag     0x00010000  //  No corresponding char.
#endif

//#include <winuser.h>
#ifndef KEYEVENTF_SILENT
#define KEYEVENTF_EXTENDEDKEY 0x0001
#define KEYEVENTF_KEYUP       0x0002
#define KEYEVENTF_SILENT      0x0004
#endif

#endif

#define _VK_TIME    0xF9
#define _VK_DATE    0xFA
#define _VK_CUT     0xFB
#define _VK_COPY    0xFC
#define _VK_PASTE   0xFD
#define _VK_MOD     0xFE
#define _VK_FN      0xFF

STDAPI DllGetClassObject(REFCLSID, REFIID, LPVOID*);
STDAPI DllCanUnloadNow();
STDAPI DllUnregisterServer();
STDAPI DllRegisterServer();
