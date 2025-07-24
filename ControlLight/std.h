#pragma once
#include <afxwin.h>
//#include <afxext.h>   // if you use MFC extensions


extern void AddErrorMessageCString(const CString &lpszText, bool dothrow = true);

extern void ControlMessageBox(const CString& lpszText, UINT nType = MB_OK, UINT nIDHelp = 0);