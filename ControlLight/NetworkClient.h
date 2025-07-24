#pragma once

//#define _AFXDLL

#include <afxwin.h>
#include "Network.h"



class CNetworkClient : public CObject
{
private:
	int mode;
	bool DebugOn;
	CString DebugFileName;
public:
	CNetwork* Network;
	void DebugStop();
	void DebugStart(CString Filename);
	void StopFastWrite();
	void StartFastWrite();
	bool FastWrite;
	void Flush();
	bool Ready();
	bool ConnectSocket(LPCTSTR lpszAddress,UINT port,CString SocketName);
	bool ReadDouble(double &Value);
	bool ReadBool(bool& Value);
	bool ReadInt(int& Value, double timeout_in_seconds = 5);
	bool ReadLong(long& Value);
	bool ReadInt64(unsigned long long& Value);
	bool SendCommand(const CString &comand);
	bool SendData(uint8_t* Data, unsigned long Size);
	bool WriteDouble(double d);
	bool WriteInteger(long i);
	bool WriteBoolean(bool b);
	bool WriteString(CString s);
	bool WriteChar(char c);
	bool Command(CString comand, bool DontWaitForReady = false);
	bool AttemptCommand(CString CommandString);
	bool GetCommand(CString &Command, double timeout_in_seconds = 5);
	CNetworkClient(int amode, bool aFastWrite = false);
	virtual ~CNetworkClient();
	void Debug(CString filename);
};
