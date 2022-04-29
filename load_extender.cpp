
#include <climits>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
// easyhook.h installed with NuGet
// https://easyhook.github.io/documentation.html
#include <easyhook.h>
#include <Windows.h>
using wcharOrChar = wchar_t; // file paths are UTF-16LE on Windows
using strType = std::wstring;
using svType = std::wstring_view;
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
using wcharOrChar = char;
using strType = std::string;
using svType = std::string_view;
#endif

// using multiple cpp files made exe bigger, so definitions are in this header
#include "shared.h"

static MapAndMutex mapAndMutexObject;

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
    const wchar_t* path = (const wchar_t*)(ObjectAttributes->ObjectName->Buffer);
    int pathEndIndex = (ObjectAttributes->ObjectName->Length) / sizeof(wchar_t);
    int filenameIndex = pathEndIndex;

    for (; filenameIndex >= 0 && path[filenameIndex] != '\\'; filenameIndex--);

    filenameIndex++; // moving past '\\' character or to 0 if no '\\' was found
    auto it = mapAndMutexObject.fileMap.find(
        svType(path + filenameIndex,
        (size_t)pathEndIndex - filenameIndex)
    );

    if (it != mapAndMutexObject.fileMap.end())
    {
        mapAndMutexObject.delayFile(it->second);
    }

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
    HOOK_TRACE_INFO hHook1 = { nullptr };
    HMODULE moduleHandle = GetModuleHandle(TEXT("ntdll"));

    if (moduleHandle)
    {
        LhInstallHook(
            GetProcAddress(moduleHandle, "NtCreateFile"),
            NtCreateFileHook,
            nullptr,
            &hHook1
        );
    }

    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hHook1);
}
#else
static auto originalFopen = reinterpret_cast<FILE * (*)(const char* path, const char* mode)>(dlsym(RTLD_NEXT, "fopen"));

FILE* fopen(const char* path, const char* mode)
{
    int filenameIndex = -1;
    int pathEndIndex = 0;

    for (; path[pathEndIndex] != '\0'; pathEndIndex++)
    {
        if (path[pathEndIndex] == '/')
        {
            filenameIndex = pathEndIndex;
        }
    }
    
    filenameIndex++; // moving past '/' character or to 0 if no '/' was found
    auto it = mapAndMutexObject.fileMap.find(
        svType(path + filenameIndex,
        (size_t)pathEndIndex - filenameIndex)
    );
    
    if (it != mapAndMutexObject.fileMap.end())
    {
        delayFile(it->second);
    }

    return original_fopen(path, mode);
}
#endif
