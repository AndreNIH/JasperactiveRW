#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <TlHelp32.h>
#include "bytecontainer.h"

class Memory {
private:
	HANDLE Process = 0;
	DWORD pID;
	DWORD GET_PID_BYNAME(const char* name);
public:
	DWORD_PTR search_bytes(std::vector<BYTE>Pattern);
	bool bind(const char* name);
	bool Write(size_t dst, std::vector<BYTE>WData);
	std::vector<BYTE> Read(size_t dst, size_t len);
	bool isBound();

	Memory(const char* target);
	Memory();
	~Memory();
};

