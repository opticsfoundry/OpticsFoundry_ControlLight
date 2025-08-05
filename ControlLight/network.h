
#pragma once

#ifdef _WINSOCKAPI_
#undef _WINSOCKAPI_ // Prevent inclusion of Winsock.h
#endif
#include <WinSock2.h>   // Include Winsock2.h before other headers
#include <ws2tcpip.h> // Optional: Include for additional Winsock functions like getaddrinfo

// Ensure that the Winsock library is linked
//#pragma comment(lib, "Ws2_32.lib")

#include <Windows.h>    // Ensure Windows.h is included after Winsock2.h

#include <afxsock.h>

#include <fstream>
using namespace std;

extern void Sleep_ms(unsigned long ms);

class CNetwork : public CObject
{
// Attributes
public:
	CSocket* m_pSocket;
	LPCTSTR m_lpszAddress;
	UINT m_nPort; 
	CString m_SocketName;
// Operations
public:
	bool ConnectSocket(LPCTSTR lpszAddress, UINT nPort,CString SocketName, bool reconnect = false, int timeout_s = 2);
	bool ResetConnection(unsigned long sleep_time=2000);
	bool FlushInputBuffer();
	void SendMsg(CString& strText);
	bool SendData(const uint8_t* Data, unsigned long Size);
	bool SendString(const CString& str);  // Helper for text-based protocols
	virtual bool ReceiveMsg(char end_character = '\n', bool WaitForStartCharacter = false, char start_character = '*', double timeout_in_seconds = 5);
	bool ReceiveData(uint8_t* buffer, unsigned long buffer_length, unsigned long timeout = 5000);
	bool ReceiveString(CString& outStr, double timeout_in_seconds = 5, char endChar = '\n');
// Implementation
public:
	void DebugStop();
	void DebugStart(CString Filename);
	ofstream *DebugFile;
	void Flush();
	void StoreLastMessage(CString Message);
	bool GetMessage(CString &Message, double timeout_in_seconds = 5, int mode = 1);
	CString LastMessage;
	CNetwork();
	virtual ~CNetwork();
	void DisconnectSocket();
private:
	bool WaitForRead(unsigned long timeout_ms);

public:
	// New reconnect/retry helpers:
	bool IsConnected() const;
	bool Reconnect(int maxRetries = 3, int timeout_s =2, unsigned long delay_ms = 1000);
	//bool SendDataWithRetry(const uint8_t* data, unsigned long size, int maxRetries = 3, unsigned long delay_ms = 1000);
	//bool ReceiveDataWithRetry(uint8_t* buffer, unsigned long size, unsigned long timeout_ms = 5000, int maxRetries = 3, unsigned long delay_ms = 1000);

	// (Optional) Track connection status
private:
	bool m_lastOpFailed = false; // for auto-reconnect, if desired
};
