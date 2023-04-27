
#include <tchar.h>
#include <iostream>
#include <format>
#include <climits>
// easyhook.h installed with NuGet
// https://easyhook.github.io/documentation.html
#include <easyhook.h>
#include <Windows.h>
#include <tlhelp32.h>

#ifdef COMPILE_FOR_AMNESIA
DWORD findAmnesiaPid()
{
	const wchar_t steamName[] = L"Amnesia.exe";
	const wchar_t nosteamName[] = L"Amnesia_NoSteam.exe";

	PROCESSENTRY32 processEntry{};
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		printf("error when using CreateToolhelp32Snapshot: %d\n", GetLastError());
		return (DWORD)-1;
	}

	if (!Process32First(snapshot, &processEntry))
	{
		printf("error when using Process32First: %d\n", GetLastError());
		CloseHandle(snapshot);
		return (DWORD)-1;
	}

	do
	{
		if (wcscmp(processEntry.szExeFile, steamName) == 0 || wcscmp(processEntry.szExeFile, nosteamName) == 0)
		{
			CloseHandle(snapshot);
			return processEntry.th32ProcessID;
		}
	} while (Process32Next(snapshot, &processEntry));

	CloseHandle(snapshot);
	printf("couldn't find amnesia process\n");
	return (DWORD)-1;
}
#endif

void getExitInput()
{
	int ch = 0;
#ifndef COMPILE_FOR_AMNESIA
	for (; ch != '\n'; ch = std::getchar());
#endif
	std::printf("Press Enter to exit\n");
	ch = std::getchar();
}

int _tmain(int argc, _TCHAR* argv[])
{
	WCHAR* dllToInject32 = nullptr;
	WCHAR* dllToInject64 = nullptr;
	_TCHAR* applicationName = argv[0];
	DWORD binaryType = 0;
	BOOL getBinaryTypeResult = GetBinaryType(applicationName, &binaryType);

	if (getBinaryTypeResult == 0 || (binaryType != 0 && binaryType != 6))
	{
		std::printf("ERROR: This exe wasn't identified as 32-bit or as 64-bit\n");
		getExitInput();
		return EXIT_FAILURE;
	}
	else if (binaryType == 0)
	{
		dllToInject32 = (WCHAR*)L"load_extender_32.dll";
	}
	else
	{
		dllToInject64 = (WCHAR*)L"load_extender_64.dll";
	}
#ifdef COMPILE_FOR_AMNESIA
	DWORD pid = findAmnesiaPid();
	if (pid == (DWORD)-1)
	{
		getExitInput();
		return EXIT_FAILURE;
	}
#else
	std::printf("Enter the process Id: ");
	DWORD pid = 0;
	std::cin >> pid;
#endif

	NTSTATUS errorCode = RhInjectLibrary(
		pid,                     // The process to inject into
		0,                       // ThreadId to wake up upon injection
		EASYHOOK_INJECT_DEFAULT,
		dllToInject32,           // 32-bit
		dllToInject64,           // 64-bit
		nullptr,                 // data to send to injected DLL entry point
		0                        // size of data to send
	);

	if (errorCode != 0)
	{
		std::printf("RhInjectLibrary failed with error code = %d\n", errorCode);
		PWCHAR errorMessage = RtlGetLastErrorString();
		std::printf("%ls\n", errorMessage);
		getExitInput();
		return EXIT_FAILURE;
	}

	std::printf("Library injected successfully.\n");
	getExitInput();
	return EXIT_SUCCESS;
}
