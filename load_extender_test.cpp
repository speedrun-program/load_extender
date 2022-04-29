
#include <climits>
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>

#ifdef _WIN32
using wcharOrChar = wchar_t; // file paths are UTF-16LE on Windows
using strType = std::wstring;
using svType = std::wstring_view;
#else
using wcharOrChar = char;
using strType = std::string;
using svType = std::string_view;
#endif

#define DEBUG
unsigned int fullResetCount = 0;

// using multiple cpp files made exe bigger, so definitions are in this header
#include "shared.h"

void windowsHookFunction(MapAndMutex& mapAndMutexObject, strType& pathStr)
{
    if (!pathStr.empty() && pathStr.back() == '\r')
    {
        pathStr.pop_back();
    }

    const wcharOrChar* path = pathStr.c_str();

    // in the actual program, this will always be less than INT_MAX
    int pathEndIndex = pathStr.size() < INT_MAX ? (int)pathStr.size() : INT_MAX;
    int filenameIndex = pathEndIndex;

    for (; filenameIndex >= 0 && path[filenameIndex] != '\\'; filenameIndex--);

    filenameIndex++; // moving past '\\' character or to 0 if no '\\' was found
    auto it = mapAndMutexObject.fileMap.find(
        svType(path + filenameIndex,
        (size_t)pathEndIndex - filenameIndex)
    );

    if (it != mapAndMutexObject.fileMap.end())
    {
#ifdef _WIN32
        printf("\n%ls found in map\n", pathStr.c_str());
#else
        printf("\n%s found in map\n", pathStr.c_str());
#endif
        mapAndMutexObject.delayFile(it->second);
    }
    else
    {
#ifdef _WIN32
        printf("\n%ls not found in map\n", pathStr.c_str());
#else
        printf("\n%s not found in map\n", pathStr.c_str());
#endif
    }
}

void unixHookFunction(MapAndMutex& mapAndMutexObject, strType& pathStr)
{
    if (!pathStr.empty() && pathStr.back() == '\r')
    {
        pathStr.pop_back();
    }

    const wcharOrChar* path = pathStr.c_str();

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
#ifdef _WIN32
        printf("\n%ls found in map\n", pathStr.c_str());
#else
        printf("\n%s found in map\n", pathStr.c_str());
#endif
        mapAndMutexObject.delayFile(it->second);
    }
    else
    {
#ifdef _WIN32
        printf("\n%ls not found in map\n", pathStr.c_str());
#else
        printf("\n%s not found in map\n", pathStr.c_str());
#endif
    }
}

void printMap(const myMapType& fileMap)
{
    for (auto& it : fileMap)
    {
        printf(
#ifdef _WIN32
            "%ls : fullResetCheckNumber %zu : position %zu : ",
#else
            "%s : fullResetCheckNumber %zu : position %zu : ",
#endif
            it.first.c_str(),
            it.second.fullResetCheckNumber,
            it.second.position
        );

        for (size_t i = 0; i < it.second.delays.size(); i++)
        {
            printf("%d / ", it.second.delays.at(i));
        }

        if (it.second.reset)
        {
            printf("RESET");
        }
        else if (it.second.resetAll)
        {
            printf("RESET ALL");
        }

        printf("\n");
    }
}

void testFunctions(MapAndMutex& mapAndMutexObject, FileHelper& fhelper, bool testUnix)
{
    strType pathStr;
    wcharOrChar ch = '\0';

    while (fhelper.getCharacter(ch))
    {
        if (ch == '\n')
        {
            if (testUnix)
            {
                unixHookFunction(mapAndMutexObject, pathStr);
            }
            else
            {
                windowsHookFunction(mapAndMutexObject, pathStr);
            }

            printMap(mapAndMutexObject.fileMap);
            pathStr.clear();
        }
        else
        {
            pathStr.push_back(ch);
        }
    }

    if (testUnix)
    {
        unixHookFunction(mapAndMutexObject, pathStr);
    }
    else
    {
        windowsHookFunction(mapAndMutexObject, pathStr);
    }
}

void testInputs(MapAndMutex& mapAndMutexObject)
{
    FileHelper fhelper;
    fhelper.tryToOpenFile("test_input.txt");
    wcharOrChar byteOrderMark = '\0';
#ifdef _WIN32

    if (!fhelper.getCharacter(byteOrderMark))
    {
        printf(
            "test_input.txt byte order mark is missing\n\
            save test_input.txt as UTF-16 LE\n\n"
        );

        return;
    }
    else if (byteOrderMark != 0xFEFF) // not 0xFFFE due to how wchar_t is read
    {
        printf(
            "test_input.txt byte order mark isn't marked as UTF-16 LE\n\
            make sure files_and_delays.txt is saved as UTF-16 LE\n\n"
        );
    }
#endif
    printf("\ntesting UNIX\n\n - - - - - - - - - -\n\n");
    printMap(mapAndMutexObject.fileMap);
    testFunctions(mapAndMutexObject, fhelper, true);
    printMap(mapAndMutexObject.fileMap);
    fullResetCount = 0;
    fhelper.resetFile();
#ifdef _WIN32
    fhelper.getCharacter(byteOrderMark);
#endif

    for (auto& it : mapAndMutexObject.fileMap)
    {
        it.second.position = 0;
        it.second.fullResetCheckNumber = 0;
    }

    printf("\n\ntesting Windows\n\n - - - - - - - - - -\n\n");
    printMap(mapAndMutexObject.fileMap);
    testFunctions(mapAndMutexObject, fhelper, false);
    printMap(mapAndMutexObject.fileMap);
}

int main()
{
    try
    {
        printf("\ntest start\n\n");
        MapAndMutex mapAndMutexObject;
        testInputs(mapAndMutexObject);
    }
    catch (char const* e)
    {
        printf("%s", e);
    }

    printf("\ntest finished, press Enter to exit\n");
    char ch = getchar();

    return 0;
}
