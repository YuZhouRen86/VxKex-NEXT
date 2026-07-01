#include "buildcfg.h"
#include "cpiwbypaldr.h"


WCHAR KexDir[MAX_PATH] = {0};
WCHAR DllPath[MAX_PATH] = {0};

PULONG InjectedPids = NULL;
INT InjectedCount = 0, InjectedCapacity = 0;
CRITICAL_SECTION CriticalSection;

INT IsPidInjected(
	DWORD Pid)
{
	INT Found = 0;
	INT Index;
	EnterCriticalSection(&CriticalSection);
	for (Index = 0; Index < InjectedCount; ++Index) {
		if (InjectedPids[Index] == Pid) {
			Found = 1;
			break;
		}
	}
	LeaveCriticalSection(&CriticalSection);
	return Found;
}

VOID AddPid(
	DWORD Pid)
{
	INT NewCapacity;
	PULONG NewBuffer;

	EnterCriticalSection(&CriticalSection);
	if (InjectedCount >= InjectedCapacity) {
		NewCapacity = (InjectedCapacity == 0) ? INITIAL_CAPACITY : InjectedCapacity * 2;
		NewBuffer = (PULONG)realloc(InjectedPids, NewCapacity * sizeof(DWORD));
		if (NewBuffer == NULL) {
			LeaveCriticalSection(&CriticalSection);
			return;
		}
		InjectedPids = NewBuffer;
		InjectedCapacity = NewCapacity;
	}
	InjectedPids[InjectedCount++] = Pid;
	LeaveCriticalSection(&CriticalSection);
}

// Internal deletion, caller must hold the critical section
static VOID RemovePidInternal(
	DWORD Pid)
{
	INT Index;
	for (Index = 0; Index < InjectedCount; ++Index) {
		if (InjectedPids[Index] == Pid) {
			InjectedPids[Index] = InjectedPids[InjectedCount - 1];
			InjectedCount--;
			break;
		}
	}

	// Shrink capacity if needed
	if (InjectedCapacity > INITIAL_CAPACITY && InjectedCount <= InjectedCapacity / 4) {
		INT NewCap = InjectedCapacity / 2;
		if (NewCap < INITIAL_CAPACITY) NewCap = INITIAL_CAPACITY;
		if (NewCap != InjectedCapacity) {
			PULONG NewBuf = (PULONG)realloc(InjectedPids, NewCap * sizeof(DWORD));
			if (NewBuf != NULL) {
				InjectedPids = NewBuf;
				InjectedCapacity = NewCap;
			}
		}
	}
}

// External deletion (with lock)
VOID RemovePid(
	DWORD Pid)
{
	EnterCriticalSection(&CriticalSection);
	RemovePidInternal(Pid);
	LeaveCriticalSection(&CriticalSection);
}

INT InjectDll(
	DWORD	Pid,
	PWSTR	DllPath)
{
	HANDLE Process;
	size_t Size;
	LPVOID Remote;
	LPTHREAD_START_ROUTINE LoadLibraryPtr;
	HANDLE Thread;

	Process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, Pid);
	if (!Process) {
		//wprintf(L"Failed to open process PID=%lu (error: %lu)\n", Pid, GetLastError());
		return 0;
	}

	Size = (wcslen(DllPath) + 1) * sizeof(WCHAR);
	Remote = VirtualAllocEx(Process, NULL, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!Remote) {
		CloseHandle(Process);
		return 0;
	}

	if (!WriteProcessMemory(Process, Remote, DllPath, Size, NULL)) {
		VirtualFreeEx(Process, Remote, 0, MEM_RELEASE);
		CloseHandle(Process);
		return 0;
	}

	LoadLibraryPtr = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
	if (!LoadLibraryPtr) {
		VirtualFreeEx(Process, Remote, 0, MEM_RELEASE);
		CloseHandle(Process);
		return 0;
	}

	Thread = CreateRemoteThread(Process, NULL, 0, LoadLibraryPtr, Remote, 0, NULL);
	if (!Thread) {
		VirtualFreeEx(Process, Remote, 0, MEM_RELEASE);
		CloseHandle(Process);
		return 0;
	}

	WaitForSingleObject(Thread, INFINITE);
	VirtualFreeEx(Process, Remote, 0, MEM_RELEASE);
	CloseHandle(Thread);
	CloseHandle(Process);
	return 1;
}

// Get all explorer.exe process IDs (dynamically allocated array)
PULONG GetExplorerPids(
	PINT	CountPtr)
{
	PULONG Pids = NULL;
	INT Count = 0;
	INT Capacity = 0;
	PROCESSENTRY32W Pe = {sizeof(Pe)};
	WCHAR WindowsDir[MAX_PATH];
	WCHAR ExplorerFullPath[MAX_PATH];
	HANDLE Snap;

	// Get the system Windows directory
	if (GetWindowsDirectoryW(WindowsDir, MAX_PATH) == 0) {
		*CountPtr = 0;
		return NULL;
	}
	// Build the expected full path: "C:\Windows\explorer.exe"
	StringCchCopy(ExplorerFullPath, MAX_PATH, WindowsDir);
	StringCchCat(ExplorerFullPath, MAX_PATH, L"\\explorer.exe");

	Snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Snap == INVALID_HANDLE_VALUE) {
		*CountPtr = 0;
		return NULL;
	}

	if (Process32FirstW(Snap, &Pe)) {
		do {
			if (StringEqualI(Pe.szExeFile, L"explorer.exe")) {
				// Open the process to query its full image path
				HANDLE Process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pe.th32ProcessID);
				if (Process) {
					WCHAR FullPath[MAX_PATH];
					DWORD PathLen = MAX_PATH;
					// Query the full image path
					if (QueryFullProcessImageNameW(Process, 0, FullPath, &PathLen)) {
						// Compare with the expected system path
						if (StringEqualI(FullPath, ExplorerFullPath)) {
							// Valid explorer.exe – add PID to list
							if (Count >= Capacity) {
								INT NewCapacity = (Capacity == 0) ? 16 : Capacity * 2;
								PULONG NewBuffer = (PULONG)realloc(Pids, NewCapacity * sizeof(DWORD));
								if (!NewBuffer) {
									CloseHandle(Process);
									break;
								}
								Pids = NewBuffer;
								Capacity = NewCapacity;
							}
							Pids[Count++] = Pe.th32ProcessID;
						}
					}
					CloseHandle(Process);
				}
			}
		} while (Process32NextW(Snap, &Pe));
	}

	CloseHandle(Snap);
	*CountPtr = Count;
	return Pids;
}

// Comparison function for qsort / bsearch
static int __cdecl CompareDword(
	const void	*a,
	const void	*b)
{
	DWORD da = *(const DWORD*)a;
	DWORD db = *(const DWORD*)b;
	return (da > db) - (da < db);
}

// Polling Thread
DWORD WINAPI PollingThread(
	LPVOID	lpParam)
{
	DWORD Pid;
	PULONG CurrentPids = NULL;
	INT Index;
	INT CurrentCount = 0;

	//wprintf(L"Polling thread started (interval = %d ms)\n", POLL_INTERVAL_MS);

	while (1) {
		BOOL IsExplorerCpiwBypassEnabled = KxCfgQueryExplorerCpiwBypass();
		CurrentPids = GetExplorerPids(&CurrentCount);

		// If no explorer.exe processes are found, clear all injection records
		if (CurrentPids == NULL || CurrentCount == 0) {
			EnterCriticalSection(&CriticalSection);
			if (InjectedCount > 0) {
				//wprintf(L"No explorer.exe processes found, clearing all injected records.\n");
				free(InjectedPids);
				InjectedPids = NULL;
				InjectedCount = 0;
				InjectedCapacity = 0;
			}
			LeaveCriticalSection(&CriticalSection);
			if (CurrentPids) free(CurrentPids);
			Sleep(POLL_INTERVAL_MS);
			continue;
		}

		// Sort current PID array for binary search
		qsort(CurrentPids, CurrentCount, sizeof(DWORD), CompareDword);

		// Detect new processes and inject
		for (Index = 0; Index < CurrentCount; ++Index) {
			Pid = CurrentPids[Index];
			if (!IsPidInjected(Pid) && IsExplorerCpiwBypassEnabled) {
				//wprintf(L"Detected new process PID=%lu, injecting...\n", Pid);
				if (InjectDll(Pid, DllPath)) {
					AddPid(Pid);
					//wprintf(L"Injection succeeded\n");
				} else {
					//wprintf(L"Injection failed\n");
				}
			}
		}

		// Check if injected processes have exited (using binary search, O(log n))
		EnterCriticalSection(&CriticalSection);
		for (Index = 0; Index < InjectedCount; ++Index) {
			PULONG Found;
			Pid = InjectedPids[Index];
			Found = (PULONG)bsearch(&Pid, CurrentPids, CurrentCount, sizeof(DWORD), CompareDword);
			if (!Found) {
				// Process exited, remove from record (call internal version to avoid deadlock)
				RemovePidInternal(Pid);
				//wprintf(L"Process PID=%lu exited, removed from record (count: %d, capacity: %d)\n", Pid, InjectedCount, InjectedCapacity);
				// Since current element was removed, decrement index
				Index--;
			}
		}
		LeaveCriticalSection(&CriticalSection);

		free(CurrentPids);
		CurrentPids = NULL;
		CurrentCount = 0;

		Sleep(POLL_INTERVAL_MS);
	}

	free(CurrentPids);
	return 0;
}

// Main Function
INT WINAPI wWinMain(
	HINSTANCE	Instance,
	HINSTANCE	PrevInstance,
	LPWSTR		CmdLine,
	INT			nCmdShow)
{
	INT Argc;
	PPWSTR Argv = CommandLineToArgvW(CmdLine, &Argc);
	if (!Argv || Argc != 1) return 0;
	if (!StringEqualI(L"/START", Argv[0])) return 0;
	LocalFree(Argv);
	if (!KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir))) return 0;
	StringCchCopy(DllPath, MAX_PATH, KexDir);
	StringCchCat(DllPath, MAX_PATH, L"\\CpiwBypa.dll");
	//wprintf(L"Target DLL: %ls\n", DllPath);
	InitializeCriticalSection(&CriticalSection);
	InjectedPids = NULL;
	InjectedCount = 0;
	InjectedCapacity = 0;
	PollingThread(NULL);
	free(InjectedPids);
	DeleteCriticalSection(&CriticalSection);
	//wprintf(L"Program terminated.\n");
	return 0;
}