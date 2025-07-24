#include "std.h"

#define _AFXDLL
#include <afxwin.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#include "ControlAPI.h"

#include <string>
#include <atlconv.h>

void AddErrorMessageCString(const CString& lpszText, bool dothrow) {
	USES_CONVERSION; // Required macro in older ATL builds
	CT2A ascii(lpszText);  // convert wide CString to narrow ANSI string
	std::string errorMessage(ascii);  // now this should compile
	AddErrorMessage(errorMessage, dothrow);
}

void ControlMessageBox(const CString& lpszText, UINT nType, UINT nIDHelp) {
#ifdef _AFXDLL
	if (DisplayErrors) AfxMessageBox(_T(lpszText), nType, nIDHelp);
#else
	if (DisplayErrors) MessageBox(NULL, lpszText, L"Control DLL", nType);
#endif
}