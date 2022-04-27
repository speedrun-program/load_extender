
#include <cstdio>
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
#include "shared_stuff.h"

void windowsHookFunction(myMapType& m, strType& pathStr, std::mutex& mutexForMap)
{
    if (!pathStr.empty() && pathStr.back() == '\r')
    {
        pathStr.pop_back();
    }

    const wcharOrChar* path = pathStr.c_str();

    int pathEndIndex = (int)pathStr.size();
    int filenameIndex = pathEndIndex;

    for (; filenameIndex >= 0 && path[filenameIndex] != '\\'; filenameIndex--);

    filenameIndex++; // moving past '\\' character or to 0 if no '\\' was found
    auto it = m.find(svType(path + filenameIndex, (size_t)pathEndIndex - filenameIndex));

    if (it != m.end())
    {
#ifdef _WIN32
        printf("\n%ls found in map\n", pathStr.c_str());
#else
        printf("\n%s found in map\n", pathStr.c_str());
#endif
        delayFile(m, it->second, mutexForMap);
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

void unixHookFunction(myMapType& m, strType& pathStr, std::mutex& mutexForMap)
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
    auto it = m.find(svType(path + filenameIndex, (size_t)pathEndIndex - filenameIndex));

    if (it != m.end())
    {
#ifdef _WIN32
        printf("\n%ls found in map\n", pathStr.c_str());
#else
        printf("\n%s found in map\n", pathStr.c_str());
#endif
        delayFile(m, it->second, mutexForMap);
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

void printMap(const myMapType& m)
{
    for (auto& it : m)
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

void testFunctions(myMapType& m, FileHelper& fh, std::mutex& mutexForMap, bool testUnix)
{
    strType pathStr;
    wcharOrChar ch = '\0';

    while (fh.getCharacter(ch))
    {
        if (ch == '\n')
        {
            if (testUnix)
            {
                unixHookFunction(m, pathStr, mutexForMap);
            }
            else
            {
                windowsHookFunction(m, pathStr, mutexForMap);
            }

            printMap(m);
            pathStr.clear();
        }
        else
        {
            pathStr.push_back(ch);
        }
    }

    if (testUnix)
    {
        unixHookFunction(m, pathStr, mutexForMap);
    }
    else
    {
        windowsHookFunction(m, pathStr, mutexForMap);
    }
}

void testInputs(myMapType& m, std::mutex& mutexForMap)
{
    FileHelper fh;
    fh.tryToOpenFile("test_input.txt");
    wcharOrChar byteOrderMark = '\0';
#ifdef _WIN32

    if (!fh.getCharacter(byteOrderMark))
    {
        printf(
            "test_input.txt byte order mark is missing\n\
            save test_input.txt as UTF-16 LE\n\n"
        );

        return;
    }
    else if (byteOrderMark != 0xFEFF) // not 0xFFFE because of how wchar_t is read
    {
        printf(
            "test_input.txt byte order mark isn't marked as UTF-16 LE\n\
            make sure files_and_delays.txt is saved as UTF-16 LE\n\n"
        );
    }
#endif
    printf("\ntesting UNIX\n\n - - - - - - - - - -\n\n");
    printMap(m);
    testFunctions(m, fh, mutexForMap, true);
    printMap(m);
    fullResetCount = 0;
    fh.resetFile();
#ifdef _WIN32
    fh.getCharacter(byteOrderMark);
#endif

    for (auto& it : m)
    {
        it.second.position = 0;
        it.second.fullResetCheckNumber = 0;
    }

    printf("\n\ntesting Windows\n\n - - - - - - - - - -\n\n");
    printMap(m);
    testFunctions(m, fh, mutexForMap, false);
    printMap(m);
}

int main()
{
    try
    {
        printf("\ntest start\n\n");
        myMapType m;
        std::mutex mutexForMap = setupMap(m);
        testInputs(m, mutexForMap);
    }
    catch (char const* e)
    {
        printf("%s", e);
    }

    printf("\ntest finished, press Enter to exit\n");
    char ch = getchar();

    return 0;
}
