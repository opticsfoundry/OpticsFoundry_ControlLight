
#include "network.h"
#include "std.h"


// Try to use CSocketException to see if the compiler sees it:
//void test_exception() {
//	CSocketException e;
//}

//#include <afxcplex.h>      // For MFC exception macros (TRY/CATCH)


#ifdef _WIN32
#ifndef _UNICODE
#include <strstream>
using namespace std;
#endif
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


void Sleep_ms(unsigned long ms) {
	unsigned  long start = GetTickCount();
	while (GetTickCount() - start < ms) {
		//AfxGetApp()->PumpMessage();
		Sleep(1); // to avoid spinning
	}
}


/////////////////////////////////////////////////////////////////////////////
// CNetwork construction/destruction

CNetwork::CNetwork()
{	
	DebugFile=NULL;
	m_pSocket = NULL;
	LastMessage="";
}

CNetwork::~CNetwork()
{
	DebugStop();	
	DisconnectSocket();
}

void CNetwork::StoreLastMessage(CString Message)
{
	if (DebugFile) (*DebugFile) << Message << endl;
}

void CNetwork::Flush()
{
	FlushInputBuffer();
	if (LastMessage != "") AddErrorMessageCString("CNetwork::Flush : Message " + LastMessage + " flushed");
	LastMessage = "";
}

void CNetwork::DebugStart(CString Filename) {
	DebugStop();
	DebugFile = new ofstream(Filename, ios::out);
}

void CNetwork::DebugStop() {
	if (DebugFile) {
		DebugFile->close();
		delete DebugFile;
		DebugFile = NULL;
	}
}

void CNetwork::DisconnectSocket()
{
	if (m_pSocket) {
		//AddErrorMessageCString("CNetwork::DisconnectSocket : Disconnecting socket ");

		StoreLastMessage("Disconnected");
		if (CAsyncSocket::LookupHandle(m_pSocket->m_hSocket, FALSE) == NULL) {
			// Avoid ASSERT by skipping Close and just invalidating the handle
			m_pSocket->m_hSocket = INVALID_SOCKET;
		}
		else {
			m_pSocket->Close();
		}
		delete m_pSocket;
		m_pSocket = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetwork Operations

//From ChatGPT: a function to connect a CSocket with a timeout
#include <afxsock.h>  // For CSocket
#include <winsock2.h> // For select, timeval, etc.
#include <ws2tcpip.h> // For inet_pton

bool ConnectWithTimeout(CSocket& sock, const CString& ipAddress, UINT port, bool reconnect, bool showError = true, int timeoutSec = 2)
{
	// 1. WSAStartup (only needed once per app, but harmless if called repeatedly)
	static bool wsaInitialized = false;
	if (!wsaInitialized) {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			AddErrorMessageCString(_T("WSAStartup failed"), /*dothrow*/ false);
			return false;
		}
		wsaInitialized = true;
	}

	// 2. Create socket
	if (!sock.Create()) {
		AddErrorMessageCString(_T("Failed to create socket"), /*dothrow*/ false);
		return false;
	}

	// 3. Set to non-blocking mode
	u_long nonBlocking = 1;
	if (ioctlsocket(sock, FIONBIO, &nonBlocking) != 0) {
		sock.Close();
		AddErrorMessageCString(_T("Failed to set non-blocking mode"), /*dothrow*/ false);
		return false;
	}

	// 4. Prepare sockaddr_in
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(CT2A(ipAddress));  // Convert CString to const char*

	// 5. Begin connection
	int result = connect(sock, (SOCKADDR*)&addr, sizeof(addr));
	if (result == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
			sock.Close();
			AddErrorMessageCString(_T("Immediate connection error"), /*dothrow*/ false);
			return false;
		}

		// 6. Wait up to timeoutSec seconds
		fd_set writeSet;
		FD_ZERO(&writeSet);
		FD_SET(sock, &writeSet);

		TIMEVAL timeout = {};
		timeout.tv_sec = timeoutSec;

		int sel = select(0, nullptr, &writeSet, nullptr, &timeout);
		if (sel <= 0 || !FD_ISSET(sock, &writeSet)) {
			sock.Close();
			CString message;
			message.Format(_T("Connection timed out or failed. IP = %s, port = %u.\n\nIf this is wrong, check the IP address given in ControlHardwareConfigFileCreator.py and run that script again.\n\nIf you don't use ControlHardwareConfig.json to configure control, check the IP given in ControlParam_SystemParamList.txt."),
				ipAddress, port);
			if (showError) AddErrorMessageCString(message);
			return false;
		}
	}

	// 7. Set back to blocking mode
	u_long blocking = 0;
	ioctlsocket(sock, FIONBIO, &blocking);

	//if (reconnect) {
	//	AddErrorMessageCString("CNetwork::ConnectWithTimeout : Reconnecting socket " + sock.m_hSocket);
	//	//  Re-attach socket to MFC for message handling
	//	sock.Attach(sock.m_hSocket); // rebinds socket to MFC message system
	//	sock.AsyncSelect(FD_READ | FD_WRITE | FD_CONNECT | FD_CLOSE);
	//}

	return true;
}


bool CNetwork::ConnectSocket(LPCTSTR lpszAddress, UINT nPort, CString SocketName, bool reconnect, int timeout_s)
{
	m_lpszAddress = lpszAddress;
	m_nPort = nPort;
	m_SocketName = SocketName;
	m_pSocket = new CSocket();
	/*if (!m_pSocket->Create()) { //socket creation is now done in ConnectWithTimeout; it's not done in CSocket::Connect
		int err = m_pSocket->GetLastError();
		CString msg;
		msg.Format(_T("Socket creation failed with error %d"), err);
		AddErrorMessageCString(msg);
		delete m_pSocket;
		m_pSocket = nullptr;
		return false;
	}*/
	
	if (!ConnectWithTimeout(*m_pSocket, m_lpszAddress, m_nPort, reconnect, (reconnect) ? false : true, timeout_s)) {
	//if (!m_pSocket->Connect(m_lpszAddress, m_nPort)) { //Standard CSocket::connect, which has a ~20s timeout
		//int err = m_pSocket->GetLastError();
		//CString msg;
		//msg.Format(_T("Socket connection to IP %s, port %u failed with error %d"), lpszAddress, nPort, err);
		//AddErrorMessageCString(msg);
		delete m_pSocket;
		m_pSocket = nullptr;
		return false;
	}
	return true;
}

bool CNetwork::ResetConnection(unsigned long sleep_time) {
	// Keep socket infrastructure alive
	CAsyncSocket dummy;
	BOOL bDummyCreated = dummy.Create();
	DisconnectSocket();
	if (sleep_time > 0) Sleep_ms(sleep_time);
	bool ret = Reconnect(/*maxRetries*/ 4,/*timeout_s*/0,/*delay_ms*/100);
	if (bDummyCreated)
		dummy.Close();
	return ret;
}

void CNetwork::SendMsg(CString& strText)
{
	StoreLastMessage(">> "+strText);

	SendString(strText);
}

bool CNetwork::SendData(const uint8_t* Data, unsigned long Size)
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	CString Message;
	Message.Format(_T(">> SendData %u"), Size);
	StoreLastMessage(Message);
	int sent = 0;
	TRY
	{		
	 sent = m_pSocket->Send(Data, Size);
	}
	CATCH(CFileException, e)
	{
		AddErrorMessageCString("CNetwork::SendData : error sending data");
		return false;
	}
	END_CATCH
	return (sent == (int)Size);
}

bool CNetwork::SendString(const CString& str) {
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	CT2A conv(str);
	const char* psz = conv;
	return SendData(reinterpret_cast<const uint8_t*>(psz), (unsigned long)strlen(psz));
}

bool CNetwork::FlushInputBuffer()
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	const int kBufferSize = 4096;
	char tempBuffer[kBufferSize];

	SOCKET s = m_pSocket->m_hSocket;

	// Set the socket temporarily to non-blocking mode
	u_long nonBlocking = 1;
	ioctlsocket(s, FIONBIO, &nonBlocking);

	int bytesRead = 0;
	do {
		bytesRead = ::recv(s, tempBuffer, kBufferSize, 0);
		if (bytesRead > 0) {
			// Data read and discarded
			tempBuffer[bytesRead + 1] = 0;
			CString buf(tempBuffer);
			StoreLastMessage("Flushed input buffer (" + buf + ")");
			continue;
		}
		else if (bytesRead == 0) {
			// Connection closed
			break;
		}
		else {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) {
				// No more data to read
				break;
			}
			else {
				CString buf;
				//buf.Format(_T("CNetwork::FlushInputBuffer :: recv() error %d"), err);
				Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
				return true;
				//ControlMessageBox(buf);
				break;
			}
		}
	} while (bytesRead > 0);

	// Restore socket to blocking mode
	nonBlocking = 0;
	ioctlsocket(s, FIONBIO, &nonBlocking);

	return true;
}

bool CNetwork::WaitForRead(unsigned long timeout_ms) {
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	fd_set readSet;
	FD_ZERO(&readSet);
	SOCKET s = m_pSocket->m_hSocket;
	FD_SET(s, &readSet);
	timeval tv;
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;
	int result = select(0, &readSet, nullptr, nullptr, &tv);
	return (result > 0) && FD_ISSET(s, &readSet);
}

bool CNetwork::ReceiveMsg(char end_character, bool WaitForStartCharacter, char start_character, double timeout_in_seconds)
{
	LastMessage = "";
	char in;
	unsigned  long timeout_ms = 1000 * timeout_in_seconds;
	if (WaitForStartCharacter) {
		in = '@';
		unsigned  long start = GetTickCount();
		while ((WaitForStartCharacter) && ((GetTickCount() - start) < timeout_ms)) {
			unsigned long timeLeft = timeout_ms - (GetTickCount() - start);
			if (!WaitForRead(timeLeft)) break;
			int nRead = m_pSocket->Receive(&in, 1);
			if (nRead != 1) {
				AddErrorMessageCString("CNetwork::ReceiveMsg : error receiving data");
				return false;
			}
			if (in != start_character) {
				CString buf;
				buf.Format(_T("CNetwork::ReceiveMsg :: start_character (%c) expected, but %c received."), start_character, in);
				AddErrorMessageCString(buf);
				return false;
			}
			else WaitForStartCharacter = false;
		}
	}
	in='@';
	unsigned  long StartTime=GetTickCount();
	ReceiveString(LastMessage, timeout_in_seconds, end_character);
	StoreLastMessage("<< "+LastMessage);
	return true;
	//AddErrorMessageCString("ReceiveMsg message received\n( " + LastMessage + ")");
}

bool CNetwork::ReceiveString(CString& outStr, double timeout_in_seconds, char endChar)
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/100);
	if (!m_pSocket) return false;
	outStr.Empty();
	char ch = 0;
	unsigned  long timeout_ms = 1000 * timeout_in_seconds;
	unsigned  long start = GetTickCount();
	while ((GetTickCount() - start) < timeout_ms) {
		unsigned long timeLeft = timeout_ms - (GetTickCount() - start);
		if (!WaitForRead(timeLeft)) break;
		int nRead = m_pSocket->Receive(&ch, 1);
		if (nRead != 1) {
			AddErrorMessageCString("CNetwork::ReceiveString : error receiving data");
			return false;
		}
		if (ch == endChar) break;
		outStr += ch;
	}
	return true;
}

bool CNetwork::GetMessage(CString& Message, double timeout_in_seconds, int mode)
{
	if (LastMessage == "") {
		unsigned  long StartTime = GetTickCount();
		unsigned  long timeout_in_ms = 1000 * timeout_in_seconds;
		while (((GetTickCount() - StartTime) < timeout_in_ms) && (LastMessage == "")) {
			if (mode == 1) ReceiveMsg(/*char end_character = */ '#', /*bool WaitForStartCharacter =*/ true, /*char start_character =*/ '*', timeout_in_seconds);
			else ReceiveMsg(/*char end_character =*/ '\n', /*bool WaitForStartCharacter = */false, /*char start_character =*/ '*', timeout_in_seconds);
		}
	}
	Message = LastMessage;
	LastMessage = "";
	return Message != "";
}

bool CNetwork::ReceiveData(uint8_t* buffer, unsigned long size, unsigned long timeout_ms)
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	unsigned long totalRead = 0;
	unsigned  long start = GetTickCount();
	while (totalRead < size && (GetTickCount() - start < timeout_ms)) {
		unsigned long timeLeft = timeout_ms - (GetTickCount() - start);
		if (!WaitForRead(timeLeft)) break;
		int nRead = 0;
		TRY
		{
			nRead = m_pSocket->Receive(buffer + totalRead, size - totalRead);
		}
			CATCH(CFileException, e)
		{
			AddErrorMessageCString("CNetwork::ReceiveData : error receiving data 1");
			return false;
		}
		END_CATCH
		if (nRead <= 0) {
			AddErrorMessageCString("CNetwork::ReceiveData : error receiving data 2");
			return false; // Disconnected or error
		}
		totalRead += nRead;
	}
	return (totalRead == size);
}

bool CNetwork::IsConnected() const {
	return (m_pSocket && m_pSocket->m_hSocket != INVALID_SOCKET);
}

bool CNetwork::Reconnect(int maxRetries, int timeout_s, unsigned long delay_ms) {

	// Keep socket infrastructure alive
	CAsyncSocket dummy;
	BOOL bDummyCreated = dummy.Create();
	DisconnectSocket();
	int tries = 0;
	while (tries < (maxRetries + 1)) {
		if (ConnectSocket(m_lpszAddress, m_nPort, m_SocketName, /*reconnect*/true,/*timeout_s*/timeout_s)) {
			StoreLastMessage("Reconnected");
			if (bDummyCreated)
				dummy.Close(); // Close dummy socket if it was created
			return true;
		}
		tries++;
		Sleep(delay_ms);
	}
	if (bDummyCreated)
		dummy.Close(); // Close dummy socket if it was created
	return false;
}


/*
bool CNetwork::Reconnect(int maxRetries, unsigned long delay_ms) {
	DisconnectSocket();
	int tries = 0;
	while (tries < maxRetries) {
		if (ConnectSocket(m_lpszAddress, m_nPort, m_SocketName))
			return true;
		tries++;
		Sleep(delay_ms);
	}
	return false;
}
*/

/*
bool CNetwork::SendDataWithRetry(const uint8_t* data, unsigned long size, int maxRetries, unsigned long delay_ms) {
	int tries = 0;
	while (tries < maxRetries) {
		TRY{
			if (SendData(data, size))
				return true;
		}
			CATCH(CSocketException, e) {
			// Optionally: log error or print here
			e->Delete();
		}
		END_CATCH
			tries++;
		if (!Reconnect(maxRetries, delay_ms))
			break; // Could not reconnect
		Sleep(delay_ms);
	}
	return false;
}

bool CNetwork::ReceiveDataWithRetry(uint8_t* buffer, unsigned long size, unsigned long timeout_ms, int maxRetries, unsigned long delay_ms) {
	int tries = 0;
	while (tries < maxRetries) {
		TRY{
			if (ReceiveData(buffer, size, timeout_ms))
				return true;
		}
			CATCH(CSocketException, e) {
			e->Delete();
		}
		END_CATCH
			tries++;
		if (!Reconnect(maxRetries, delay_ms))
			break;
		Sleep(delay_ms);
	}
	return false;
}
*/