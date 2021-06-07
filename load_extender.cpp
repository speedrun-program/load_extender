#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <filesystem>
#include <memory>
#include <climits>

#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
#define THIS_IS_NOT_THE_TEST_PROGRAM
#endif

#ifdef _WIN32
using wstring_or_string = std::wstring;
#include <Windows.h>
// easyhook.h installed with NuGet
// https://easyhook.github.io/documentation.html
#include <easyhook.h>
#else
using wstring_or_string = std::string;
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#endif

#include "robin_hood.h"
#include "other_functions.h"
// thank you for making this hash map, martinus.
// here's his github: https://github.com/martinus/robin-hood-hashing

static robin_hood::unordered_node_map<wstring_or_string, std::unique_ptr<int[]>> rh_map;
static std::mutex delay_array_mutex = read_delays_file(rh_map);

#ifdef _WIN32
static NTSTATUS WINAPI NtCreateFileHook(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength)
{
    const std::wstring& file_path = ObjectAttributes->ObjectName->Buffer;
    int file_name_index = file_path.length() - 1;
    for (; file_name_index >= 0 && file_path[file_name_index] != L'\\'; file_name_index--) {}
    // +1 so '\' isn't included. If '\' isn't found, file_name_index increases from -1 to 0;
    delay_file(rh_map, &file_path[file_name_index + 1], delay_array_mutex);
    return NtCreateFile(
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength
    );
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo);

void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
    HOOK_TRACE_INFO hHook1 = {NULL};
    LhInstallHook(
        GetProcAddress(GetModuleHandle(TEXT("ntdll")), "NtCreateFile"),
        NtCreateFileHook,
        NULL,
        &hHook1
    );
    ULONG ACLEntries[1] = {0};
    LhSetExclusiveACL(ACLEntries, 1, &hHook1);
}
#else
static auto original_fopen = reinterpret_cast<FILE * (*)(const char* path, const char* mode)>(dlsym(RTLD_NEXT, "fopen"));

FILE* fopen(const char* path, const char* mode)
{
    // finding the part of the path with the file name
    int file_name_index = -1;
    for (int i = 0; path[i] != '\0'; i++)
    {
        if (path[i] == '/')
        {
            file_name_index = i;
        }
    }
    // +1 so '\' isn't included. If '\' isn't found, file_name_index increases from -1 to 0;
    delay_file(rh_map, std::string(&path[file_name_index + 1]), delay_array_mutex);
    return original_fopen(path, mode);
}
#endif

