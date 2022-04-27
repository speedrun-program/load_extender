
#include <tchar.h>
#include <cstdio>
// easyhook.h installed with NuGet
// https://easyhook.github.io/documentation.html
#include <easyhook.h>
#include <Windows.h>

void getExitInput(char ch)
{
	for (; ch != '\n'; ch = std::getchar());

	printf("Press Enter to exit\n");
	ch = std::getchar();
}

int _tmain(int argc, _TCHAR* argv[])
{
	WCHAR* dllToInject32 = nullptr;
	WCHAR* dllToInject64 = nullptr;
	_TCHAR* lpApplicationName = argv[0];
	DWORD lpBinaryType = 0;

	if (GetBinaryType(lpApplicationName, &lpBinaryType) == 0 || (lpBinaryType != 0 && lpBinaryType != 6))
	{
		std::printf("ERROR: This exe wasn't identified as 32-bit or as 64-bit\n");
		getExitInput('\n');
		return 0;
	}
	else if (lpBinaryType == 0)
	{
		dllToInject32 = (WCHAR*)L"load_extender_32.dll";
	}
	else
	{
		dllToInject64 = (WCHAR*)L"load_extender_64.dll";
	}

	std::printf("Enter the process Id: ");
	long long int PIDLongLong = 0;
	char ch = std::getchar();

	for (; ch != '\n'; ch = std::getchar())
	{
		if (ch >= '0' && ch <= '9')
		{
			ch = ch - '0'; // this prevents a warning message
			PIDLongLong *= 10;
			PIDLongLong += ch;
		}

		if (PIDLongLong > 4294967295)
		{
			std::printf("PID too large\n");
			getExitInput(ch);
			return 0;
		}
	}

	NTSTATUS nt = RhInjectLibrary(
		(DWORD)PIDLongLong,      // The process to inject into
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
		getExitInput(ch);
		return 0;
	}

	std::printf("Library injected successfully.\n");
	getExitInput(ch);
	return 0;
}
