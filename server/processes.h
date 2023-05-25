#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <sstream>

struct handle_data {
	unsigned long process_id;
	HWND window_handle;
};

BOOL is_main_window(HWND handle) {   
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam) {
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle))
		return TRUE;
	data.window_handle = handle;
	return FALSE;   
}

HWND find_main_window(unsigned long process_id) {
	handle_data data;
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&data);
	return data.window_handle;
}

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1
std::vector<ProcessInfo> get_current_processes() {
	HANDLE hSnapshot;
	PROCESSENTRY32 pe;
	BOOL hResult;
	std::vector<ProcessInfo> result;

	// snapshot of all processes in the system
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) return {};

	// initializing size: needed for using Process32First
	pe.dwSize = sizeof(PROCESSENTRY32);

	// info about first process encountered in a system snapshot
	hResult = Process32First(hSnapshot, &pe);

	// retrieve information about the processes
	// and exit if unsuccessful
	while (hResult) {
		char type = 0;
		if (IsWindowVisible(find_main_window(pe.th32ProcessID))) {
			type = 1;
		}
		if (pe.th32ProcessID > 4) { //Skip some system processes
			result.push_back({(int)pe.th32ProcessID, std::string(pe.szExeFile), type});
		}
		hResult = Process32Next(hSnapshot, &pe);
	}

	// closes an open handle (CreateToolhelp32Snapshot)
	CloseHandle(hSnapshot);
	return result;
}

BOOL PauseResumeThreadList(DWORD dwOwnerPID, bool bResumeThread) { 
	HANDLE hThreadSnap; 
	BOOL bRet = FALSE; 
	THREADENTRY32 te32; 

	// Take a snapshot of all threads currently in the system. 
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 
	if (hThreadSnap == INVALID_HANDLE_VALUE) 
		return (FALSE); 

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32); 

	// Walk the thread snapshot to find all threads of the process. 
	// If the thread belongs to the process, add its information 
	// to the display list.
	if (Thread32First(hThreadSnap, &te32)) { 
		do  { 
			if (te32.th32OwnerProcessID == dwOwnerPID)  {
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
				if (bResumeThread)
					ResumeThread(hThread);
				else
					SuspendThread(hThread);
				CloseHandle(hThread);
			} 
		} while (Thread32Next(hThreadSnap, &te32)); 
		bRet = TRUE; 
	} 
	else 
		bRet = FALSE; // could not walk the list of threads 

	// Do not forget to clean up the snapshot object. 
	CloseHandle(hThreadSnap); 

	return (bRet); 
}

BOOL TerminateProcessEx(DWORD dwProcessId, UINT uExitCode) {
	DWORD dwDesiredAccess = PROCESS_TERMINATE;
	BOOL bInheritHandle = FALSE;
	HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
	if (hProcess == NULL)
		return FALSE;

	BOOL result = TerminateProcess(hProcess, uExitCode);

	CloseHandle(hProcess);

	return result;
}

void suspend_process(int processId) {
	PauseResumeThreadList(processId, 0);
}

void resume_process(int processId) {
	PauseResumeThreadList(processId, 1);
}

void terminate_process(int processId) {
	TerminateProcessEx(processId, -1);
}