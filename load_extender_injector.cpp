
// REMINDER:
// std::format blows up exe size by over 150 KB,
// so keep std::printf for formatting for now,
// then replace it and std::cout with std::print once it's available

#include <tchar.h>
#include <iostream>
#include <format>
#include <climits>
// easyhook.h installed with NuGet
// https://easyhook.github.io/documentation.html
#include <easyhook.h>
#include <Windows.h>

void getExitInput()
{
	int ch = 0;

	for (; ch != '\n'; ch = std::getchar());

	std::cout << "Press Enter to exit\n";
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
		std::cout << "ERROR: This exe wasn't identified as 32-bit or as 64-bit\n";
		getExitInput();
		return 0;
	}
	else if (binaryType == 0)
	{
		dllToInject32 = (WCHAR*)L"load_extender_32.dll";
	}
	else
	{
		dllToInject64 = (WCHAR*)L"load_extender_64.dll";
	}

	std::cout << "Enter the process Id: ";
	DWORD pid = 0;
	std::cin >> pid;

	NTSTATUS nt = RhInjectLibrary(
		pid,                     // The process to inject into
		0,                       // ThreadId to wake up upon injection
		EASYHOOK_INJECT_DEFAULT,
		dllToInject32,           // 32-bit
		dllToInject64,           // 64-bit
		nullptr,                 // data to send to injected DLL entry point
		0                        // size of data to send
	);

	if (nt != 0)
	{
		std::printf("RhInjectLibrary failed with error code = %d\n", nt);
		PWCHAR err = RtlGetLastErrorString();
		std::printf("%ls\n", err);
		getExitInput();
		return 0;
	}

	std::cout << "Library injected successfully.\n";
	getExitInput();
	return 0;
}
