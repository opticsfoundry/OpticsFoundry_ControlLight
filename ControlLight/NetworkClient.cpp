// CNetworkClient.cpp: implementation of the CNetworkClient class.
//
//////////////////////////////////////////////////////////////////////
#include <afxwin.h>
#include "NetworkClient.h"
#include "std.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNetworkClient::CNetworkClient(int amode, bool aFastWrite)
{
	mode=amode;
	Network=NULL;
	FastWrite= aFastWrite;
	DebugOn=false;
	DebugFileName = "";
}

CNetworkClient::~CNetworkClient()
{
	if (Network) {
		if (mode==1) {
			CString help="Goodbye";
			SendCommand(help);
		}
		delete Network;
	}
}

void CNetworkClient::Debug(CString filename) {
	DebugFileName = filename;
	DebugOn = true;
	if (Network) Network->DebugStart(DebugFileName);
}

bool CNetworkClient::ConnectSocket(LPCTSTR lpszAddress,UINT port,CString SocketName) {
	Network=new CNetwork();
	if (DebugOn) Network->DebugStart(DebugFileName);
	bool Connected=Network->ConnectSocket(lpszAddress,port,SocketName);
	//we insist that a server is present on program start. If it isn't we ignore that server, gaining speed. The alternative would be to try to reconnect every time, which takes a second or so, if the sever is not present.
	//if you dont want this, comment out the next four lines
	if (!Connected) {
		delete Network;
		Network=NULL;
	}
	return Connected;
}

bool CNetworkClient::SendCommand(const CString& command) {
	if (Network) {
		if (mode == 1) {
			CString msg = _T("*") + command + _T("#");
			Network->SendMsg(msg);
		} 
		else if (mode == 2) {
			unsigned int StrLength = command.GetLength();
			if (StrLength > 255) return false;
			uint8_t length = StrLength;
			Network->SendData(&length, 1);			
			Network->SendData((uint8_t*)(LPCTSTR)command, length);
		}
		else {  //mode == 3
			CString msg = command + _T("\n");
			Network->SendMsg(msg);
		}
	}
	return true;
}

bool CNetworkClient::WriteDouble(double d) {
	CString buf;
	buf.Format("%8.7e",d);
	return SendCommand(buf);
}

bool CNetworkClient::SendData(uint8_t* Data, unsigned long Size) {
	if (Network) {
		//Network->Flush();
		return Network->SendData(Data, Size);
	}
	else return true;
}

bool CNetworkClient::WriteInteger(long i) {
	CString buf;
	buf.Format("%8i",i);
	return SendCommand(buf);
}

bool CNetworkClient::WriteBoolean(bool b) {
	if (mode == 1) {
		if (b) return SendCommand("TRUE");
		else return SendCommand("FALSE");
	} else {  // mode == 2 or 3
		if (b) return SendCommand("1");
		else return SendCommand("0");
	}
}

bool CNetworkClient::WriteString(CString s) {
  return SendCommand(s);
}

bool CNetworkClient::WriteChar(char c) {
	CString buf=c;
	return SendCommand(buf);
}

bool CNetworkClient::ReadDouble(double& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	//Value=atof(buf);
	// Convert CString ? const char* safely
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
	char* endptr = nullptr;
	Value = std::strtod(str, &endptr);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadDouble : Conversion error: no digits found in ("+buf+").");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadDouble : Conversion error: leftover characters after number in (" + buf + ").");
		return false;
	}
	return ok;
}

bool CNetworkClient::ReadBool(bool& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	Value = buf == "1";
	return ok;
}

bool CNetworkClient::ReadInt(int& Value, double timeout_in_seconds)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf, timeout_in_seconds);
	//Value = atoi(buf);
	// Convert CString ? const char* safely
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
	char* endptr = nullptr;
	Value = std::strtol(str, &endptr, 10);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadInt : Conversion error: no digits found in (" + buf + ").");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadInt : Conversion error: leftover characters after number in (" + buf + ").");
		return false;
	}
	return ok;
}

bool CNetworkClient::ReadLong(long& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	//Value = atoi(buf);
	// Convert CString ? const char* safely
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
	char* endptr = nullptr;
	Value = std::strtol(str, &endptr, 10);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadLong : Conversion error: no digits found.");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadLong : Conversion error: leftover characters after number.");
		return false;
	}
	return ok;
}

bool CNetworkClient::ReadInt64(unsigned long long& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	//Value = atoi(buf);

	// Convert CString ? const char* safely
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
	char* endptr = nullptr;
	Value = std::strtoull(str, &endptr, 10);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadInt64 : Conversion error: no digits found.");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadInt64 : Conversion error: leftover characters after number.");
		return false;
	}
	return ok;
}

constexpr unsigned int MaxReconnectAttempts = 100;
bool CNetworkClient::Command(CString CommandString, bool DontWaitForReady) {
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptCommand(CommandString, DontWaitForReady))) {
		Network->ResetConnection();
		Sleep_ms(100);
		attempts++;
	}
	if (attempts == MaxReconnectAttempts) AddErrorMessageCString("CNetworkClient::Command : Maximum reconnect attempts reached. Command failed: " + CommandString);
	return (attempts < MaxReconnectAttempts);
}

bool CNetworkClient::AttemptCommand(CString comand, bool DontWaitForReady) {  
	if (Network) Network->Flush();
	SendCommand(comand);
	if ((FastWrite) || (DontWaitForReady)) return true;
	if ((!Ready()) && (Network)) {
		//AddErrorMessageCString("CNetworkClient not Ready!\n(Command: "+comand+")");
		return false;
	} else return true;
}

bool CNetworkClient::GetCommand(CString &Command, double timeout_in_seconds)
{
	if (!Network) return false;
	if (mode == 2) {
		return Network->ReceiveString(Command, timeout_in_seconds);
	} else return Network->GetMessage(Command, timeout_in_seconds, mode);
}

bool CNetworkClient::Ready() {
	//if (mode == 3) return true;
	if (Network) {
		CString buf;
		if (GetCommand(buf)) return buf=="Ready";
		else return false;
	} else return true;
}

void CNetworkClient::Flush()
{
	if (Network) Network->Flush();
}

void CNetworkClient::StartFastWrite()
{
	FastWrite=true;
}

void CNetworkClient::StopFastWrite()
{
	FastWrite=false;
	Flush();
}

void CNetworkClient::DebugStop() {
	if (Network) Network->DebugStop();
}
void CNetworkClient::DebugStart(CString Filename) {
	if (Network) Network->DebugStart(Filename);
}