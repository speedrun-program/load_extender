#include <tchar.h>
#include <iostream>
#include <string>
#include <Windows.h>
// easyhook.h installed with NuGet
// https://easyhook.github.io/documentation.html
#include <easyhook.h>

void get_exit_input()
{
	std::wcout << "Press Enter to exit";
	std::wstring input;
	std::getline(std::wcin, input);
	std::getline(std::wcin, input);
}

int _tmain(int argc, _TCHAR* argv[])
{
	WCHAR* dllToInject32 = NULL;
	WCHAR* dllToInject64 = NULL;
	LPCWSTR lpApplicationName = argv[0];
	DWORD lpBinaryType;
	if (GetBinaryType(lpApplicationName, &lpBinaryType) == 0 || (lpBinaryType != 0 && lpBinaryType != 6))
	{
		std::wcout << "ERROR: This exe wasn't identified as 32-bit or as 64-bit";
		get_exit_input();
		return 1;
	}
	else if (lpBinaryType == 0)
	{
		dllToInject32 = (WCHAR*)L"load_extender_32.dll";
	}
	else
	{
		dllToInject64 = (WCHAR*)L"load_extender_64.dll";
	}
	DWORD processId;
	std::wcout << "Enter the target process Id: ";
	std::cin >> processId;
	wprintf(L"Attempting to inject dll\n\n");
	// Inject dllToInject into the target process Id, passing 
	// freqOffset as the pass through data.
	NTSTATUS nt = RhInjectLibrary(
		processId,   // The process to inject into
		0,           // ThreadId to wake up upon injection
		EASYHOOK_INJECT_DEFAULT,
		dllToInject32, // 32-bit
		dllToInject64, // 64-bit
		NULL, // data to send to injected DLL entry point
		0 // size of data to send
	);
	if (nt != 0)
	{
		printf("RhInjectLibrary failed with error code = %d\n", nt);
		PWCHAR err = RtlGetLastErrorString();
		std::wcout << err << "\n";
		get_exit_input();
		return 1;
	}
	else
	{
		std::wcout << L"Library injected successfully.\n";
	}
	get_exit_input();
	return 0;
}
