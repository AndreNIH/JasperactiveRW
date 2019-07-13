#include "man_memory.h"

DWORD Memory::GET_PID_BYNAME (const char* name) {
	PROCESSENTRY32 PE32;
	PE32.dwSize = sizeof(PROCESSENTRY32);
	HANDLE PSNAP = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (PSNAP == 0 || !Process32First(PSNAP, &PE32)) return 0;
	do {
		if (strcmp(PE32.szExeFile, name) == 0) {
			CloseHandle(PSNAP);
			return PE32.th32ProcessID;
		}
	} while (Process32Next(PSNAP, &PE32));
	return 0;
}

DWORD_PTR Memory::search_bytes(std::vector<BYTE>Pattern){
	if (Process == INVALID_HANDLE_VALUE) return 0;
	MEMORY_BASIC_INFORMATION MBI = { 0 };
	LPVOID SysStart = 0;
	SYSTEM_INFO Sys = { 0 };
	GetSystemInfo(&Sys);
	SysStart = Sys.lpMinimumApplicationAddress;
	while (VirtualQueryEx(Process, SysStart, &MBI, sizeof(MBI))) {
		if ((MBI.State & MEM_COMMIT) && (MBI.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
			std::shared_ptr<BYTE[]>buf(new unsigned char[MBI.RegionSize]);
			ReadProcessMemory(Process, MBI.BaseAddress, buf.get(), MBI.RegionSize, 0);
			Bytecontainer memory_view(std::move(buf), MBI.RegionSize);
			auto Offset = std::search(memory_view.begin(), memory_view.end(), Pattern.begin(), Pattern.end());
			if (Offset != memory_view.end())  return reinterpret_cast<DWORD_PTR>(MBI.BaseAddress) + (Offset - memory_view.begin());
		}
		SysStart = reinterpret_cast<void*>((reinterpret_cast<DWORD>((SysStart)) + MBI.RegionSize));
	}
	return 0;
}

bool Memory::bind(const char* name) {
	if (Process) return true;
	pID = GET_PID_BYNAME(name);
	Process = OpenProcess(PROCESS_ALL_ACCESS, false, pID);
	return Process != 0;
}

bool Memory::Write(size_t dst,std::vector<BYTE> WData){
	WData.push_back(0x00);//Line especifically for this project;
	for (BYTE b : WData) {
		if (WriteProcessMemory(Process, reinterpret_cast<void*>(dst), &b, sizeof(b), 0) == false) return false;
		dst++;
	}
	return true;
}


std::vector<BYTE> Memory::Read(size_t dst, size_t len){
	std::vector<BYTE>Results;
	BYTE buffer;
	if (!ReadProcessMemory(Process, &dst, &buffer, sizeof(BYTE), 0)) return std::vector<BYTE>{};
}

bool Memory::isBound(){return Process != 0;}

Memory::Memory(const char* target) : pID(GET_PID_BYNAME(target)) {
	Process = OpenProcess(PROCESS_ALL_ACCESS, false, pID);
}

Memory::Memory() { ; }

Memory::~Memory() {
	if (Process) CloseHandle(Process);
}
