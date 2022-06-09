
#include <cstdio>
#include <climits>
#include <mutex>
#include <vector>
#include <memory>
#include <cstring>
#include <charconv>
#include <exception>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
using wcharOrChar = wchar_t; // file paths are UTF-16LE on Windows
using svType = std::wstring_view;
#define cmpFunction std::wcscmp
#else
using wcharOrChar = char;
using svType = std::string_view;
#define cmpFunction std::strcmp
#endif

using uPtrType = std::unique_ptr<wcharOrChar[]>;
using vectorType = std::vector<wcharOrChar>;

#define DEBUG
#define stdprintf std::printf
size_t fullResetCount = 0;

// using multiple cpp files made exe bigger, so definitions are in this header
#include "shared.h"

void windowsHookFunction(MapAndMutex& mapAndMutexObject, vectorType& path)
{
    if (!path.empty() && path.back() == '\r')
    {
        path.pop_back();
    }

    // in the actual program, pathEndIndex will always be less than INT_MAX
    int pathEndIndex = path.size() < INT_MAX ? (int)path.size() : INT_MAX;
    int filenameIndex = pathEndIndex - 1;
    path.push_back('\0');
    const wcharOrChar* pathPtr = path.data();

    for (; filenameIndex >= 0 && pathPtr[filenameIndex] != '\\'; filenameIndex--);

    filenameIndex++; // moving past '\\' character or to 0 if no '\\' was found
    auto it = mapAndMutexObject.fileMap.find(
        svType(pathPtr + filenameIndex,
            (size_t)pathEndIndex - filenameIndex)
    );

    if (it != mapAndMutexObject.fileMap.end())
    {
#ifdef _WIN32
        std::printf("\n%ls found in map\n", path.data());
#else
        std::printf("\n%s found in map\n", path.data());
#endif
        mapAndMutexObject.delayFile(it->second);
    }
    else
    {
#ifdef _WIN32
        std::printf("\n%ls not found in map\n", path.data());
#else
        std::printf("\n%s not found in map\n", path.data());
#endif
    }
}

void unixHookFunction(MapAndMutex& mapAndMutexObject, vectorType& path)
{
    if (!path.empty() && path.back() == '\r')
    {
        path.pop_back();
    }

    path.push_back('\0');
    const wcharOrChar* pathPtr = path.data();

    int filenameIndex = -1;
    int pathEndIndex = 0;

    // in the actual program, pathEndIndex will always be less than INT_MAX
    for (; pathEndIndex < INT_MAX && pathPtr[pathEndIndex] != '\0'; pathEndIndex++)
    {
        if (pathPtr[pathEndIndex] == '/')
        {
            filenameIndex = pathEndIndex;
        }
    }

    filenameIndex++; // moving past '/' character or to 0 if no '/' was found
    auto it = mapAndMutexObject.fileMap.find(
        svType(pathPtr + filenameIndex,
            (size_t)pathEndIndex - filenameIndex)
    );

    if (it != mapAndMutexObject.fileMap.end())
    {
#ifdef _WIN32
        std::printf("\n%ls found in map\n", path.data());
#else
        std::printf("\n%s found in map\n", path.data());
#endif
        mapAndMutexObject.delayFile(it->second);
    }
    else
    {
#ifdef _WIN32
        std::printf("\n%ls not found in map\n", path.data());
#else
        std::printf("\n%s not found in map\n", path.data());
#endif
    }
}

void printMap(const myMapType& fileMap)
{
    for (auto& [key, value] : fileMap)
    {
        std::printf(
#ifdef _WIN32
            "%ls : fullResetCheckNumber %zu : position %zu : ",
#else
            "%s : fullResetCheckNumber %zu : position %zu : ",
#endif
            key.get(),
            value.fullResetCheckNumber,
            value.position
        );

        size_t delaysPosition = 0;

        for (; value.delays[delaysPosition] >= 0; delaysPosition++)
        {
            std::printf("%d / ", value.delays[delaysPosition]);
        }

        if (value.delays[delaysPosition] == -1)
        {
            std::printf(delaysPosition == 0 ? "RESET ALL" : "RESET");
        }

        std::printf("\n");
    }
}

void testFunctions(MapAndMutex& mapAndMutexObject, FileHelper& fhelper, bool testUnix)
{
    vectorType path;
    wcharOrChar ch = '\0';

    while (fhelper.getCharacter(ch))
    {
        if (ch == '\n')
        {
            if (testUnix)
            {
                unixHookFunction(mapAndMutexObject, path);
            }
            else
            {
                windowsHookFunction(mapAndMutexObject, path);
            }

            printMap(mapAndMutexObject.fileMap);
            path.clear();
        }
        else
        {
            path.push_back(ch);
        }
    }

    if (testUnix)
    {
        unixHookFunction(mapAndMutexObject, path);
    }
    else
    {
        windowsHookFunction(mapAndMutexObject, path);
    }
}

void testInputs(MapAndMutex& mapAndMutexObject)
{
    FileHelper fhelper("test_input.txt");
#ifdef _WIN32
    wcharOrChar byteOrderMark = '\0';

    if (!fhelper.getCharacter(byteOrderMark))
    {
        std::printf(
            "test_input.txt byte order mark is missing\n\
save test_input.txt as UTF-16 LE\n\n"
);

        return;
    }
    else if (byteOrderMark != 0xFEFF) // not 0xFFFE due to how wchar_t is read
    {
        std::printf(
            "test_input.txt byte order mark isn't marked as UTF-16 LE\n\
make sure files_and_delays.txt is saved as UTF-16 LE\n\n"
);
    }
#endif

    std::printf("\ntesting UNIX\n\n - - - - - - - - - -\n\n");
    printMap(mapAndMutexObject.fileMap);
    testFunctions(mapAndMutexObject, fhelper, true);
    printMap(mapAndMutexObject.fileMap);

    // resetting things to test windows version of fake hook function
    fullResetCount = 0;
    fhelper.resetFile();
#ifdef _WIN32
    fhelper.getCharacter(byteOrderMark);
#endif

    for (auto& [key, value] : mapAndMutexObject.fileMap)
    {
        value.position = 0;
        value.fullResetCheckNumber = 0;
    }

    std::printf("\n\ntesting Windows\n\n - - - - - - - - - -\n\n");
    printMap(mapAndMutexObject.fileMap);
    testFunctions(mapAndMutexObject, fhelper, false);
    printMap(mapAndMutexObject.fileMap);
}

int main()
{
    try
    {
        std::printf("\ntest start\n\n");
        MapAndMutex mapAndMutexObject;
        testInputs(mapAndMutexObject);
    }
    catch (const std::runtime_error& e)
    {
        std::printf("%s", e.what());
        return EXIT_FAILURE;
    }

    std::printf("\ntest finished, press Enter to exit\n");
    char ch = getchar();

    return EXIT_SUCCESS;
}
