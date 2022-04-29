
// REMINDER:
// std::format blows up exe size by over 150 KB,
// so keep std::printf for formatting for now,
// then replace it and std::cout with std::print once it's available

#ifndef DEBUG
#define printf(...) (0)
#define stdcout(x) (0)
#endif

class FileHelper
{
public:
    FileHelper(const FileHelper& fhelper) = delete;
    FileHelper& operator=(FileHelper other) = delete;
    FileHelper(FileHelper&&) = delete;
    FileHelper& operator=(FileHelper&&) = delete;

    FileHelper() {}

    ~FileHelper()
    {
        if (_f)
        {
            std::fclose(_f);
        }
    }

    void tryToOpenFile(const char* filename)
    {
#ifdef _WIN32
        if (fopen_s(&_f, filename, "rb") != 0 || !_f)
#else
        if (!(_f = std::fopen(filePath, "rb")))
#endif
        {
            throw "FileHelper fopen failure in tryToOpenFile";
        }
    }

    bool getCharacter(wcharOrChar& ch)
    {
        if (_bufferPosition == _charactersRead)
        {
            _bufferPosition = 0;
            _charactersRead = (int)std::fread(_buffer.data(), sizeof(wcharOrChar), _buffer.size(), _f);

            if (!_charactersRead)
            {
                return false;
            }
        }

        ch = _buffer.at(_bufferPosition);
        _bufferPosition++;

        return true;
    }

    void resetFile() // used in tests
    {
        if (std::fseek(_f, 0, SEEK_SET) != 0)
        {
            throw "FileHelper fseek failure in resetFile";
        }

        _bufferPosition = 0;
        _charactersRead = 0;
    }

private:
    FILE* _f = nullptr;
    std::vector<wcharOrChar> _buffer = std::vector<wcharOrChar>(8192 / sizeof(wcharOrChar));
    int _bufferPosition = 0;
    int _charactersRead = 0;
};

struct MapValue
{
    std::vector<int> delays;
    size_t position = 0;
    unsigned int fullResetCheckNumber = 0;
    bool reset = false;
    bool resetAll = false;

    MapValue(bool& textRemaining, std::vector<int>& delaysVector, FileHelper& fhelper)
    {
        wcharOrChar ch = '\0';
        long long int delay = 0;

        for (
            textRemaining = fhelper.getCharacter(ch);
            ch != '\n' && textRemaining;
            textRemaining = fhelper.getCharacter(ch))
        {
            if (ch >= '0' && ch <= '9')
            {
                ch = ch - '0';
                delay *= 10;
                delay += ch;

                if (delay > INT_MAX)
                {
                    throw "delays can't be larger than INT_MAX";
                }
            }
            else if (ch == '-')
            {
                if (delaysVector.empty())
                {
                    resetAll = true;
                }
                else
                {
                    reset = true;
                }

                break;
            }
            else if (ch == '/')
            {
                delaysVector.push_back((int)delay);
                delay = 0;
            }
        }

        if (delay != 0 && !reset && !resetAll)
        {
            delaysVector.push_back((int)delay);
        }

        // make sure to go to end of line
        for (; ch != '\n' && textRemaining; textRemaining = fhelper.getCharacter(ch));
    }
};

struct KeyCmp
{
    using is_transparent = void;

    bool operator()(const svType sv1, const svType sv2) const
    {
        return sv1 == sv2;
    }
};

struct KeyHash
{
    using is_transparent = void;

    size_t operator()(const svType sv) const
    {
        return _hashObject(sv);
    }

private:
    std::hash<svType> _hashObject = std::hash<svType>();
};

using myMapType = std::unordered_map<strType, MapValue, KeyHash, KeyCmp>;

class MapAndMutex
{
public:
    std::mutex mutexForMap;
    myMapType fileMap;

    MapAndMutex()
    {
        try
        {
            std::wstring keyStr;
            std::vector<int> delaysVector;
            FileHelper fhelper;
            std::string name("files_and_delays.txt");
            size_t fileNumber = 0;

            for (
                ;
                std::filesystem::exists("files_and_delays" + std::to_string(fileNumber) + ".txt");
                fileNumber++);

            if (fileNumber > 0)
            {
                name = "files_and_delays" + std::to_string(fileNumber - 1) + ".txt";
            }

            fhelper.tryToOpenFile(name.c_str());
#ifdef _WIN32
            wchar_t byteOrderMark = '\0';

            if (!fhelper.getCharacter(byteOrderMark))
            {
                stdcout(
                    "files_and_delays.txt byte order mark is missing\n\
                make sure files_and_delays.txt is saved as UTF-16 LE\n"
                );
            }
            else if (byteOrderMark != 0xFEFF) // not 0xFFFE due to how wchar_t is read
            {
                stdcout(
                    "files_and_delays.txt byte order mark isn't marked as UTF-16 LE\n\
                make sure files_and_delays.txt is saved as UTF-16 LE\n"
                );
            }
#endif

            while (addMapPair(fileMap, keyStr, delaysVector, fhelper));
        }
        catch (char const* e)
        {
            char const* resolveC4101Warning = e;
            printf("%s\n", resolveC4101Warning);
            fileMap.clear(); // clear map so failure is more obvious
        }
    }

    bool addMapPair(myMapType& fileMap, strType& keyStr, std::vector<int>& delaysVector, FileHelper& fhelper)
    {
        keyStr.clear();
        delaysVector.clear();
        wcharOrChar ch = '\0';
        bool keepWhitespace = false;
        bool textRemaining = fhelper.getCharacter(ch);

        if (ch == '\n' || !textRemaining)
        {
            return textRemaining;
        }
        else if (ch == '/')
        {
            keepWhitespace = true;
        }
        else if (ch != ' ' && ch != '\f' && ch != '\r' && ch != '\t' && ch != '\v')
        {
            keyStr.push_back(ch);
        }

        for (
            textRemaining = fhelper.getCharacter(ch);
            ch != '\n' && ch != '/' && textRemaining;
            textRemaining = fhelper.getCharacter(ch))
        {
            if (keepWhitespace || (ch != ' ' && ch != '\f' && ch != '\r' && ch != '\t' && ch != '\v'))
            {
                keyStr.push_back(ch);
            }
        }

        if (textRemaining && ch == '/')
        {
            MapValue fileMapValue(textRemaining, delaysVector, fhelper);

            if (!keyStr.empty() && (!delaysVector.empty() || fileMapValue.resetAll))
            {
                fileMapValue.delays = delaysVector;
                strType keyStrCopy(keyStr);
                fileMapValue.delays.shrink_to_fit();
                keyStrCopy.shrink_to_fit();
                fileMap.emplace(std::move(keyStrCopy), std::move(fileMapValue));
            }
        }

        return textRemaining;
    }

    void delayFile(MapValue& fileMapValue)
    {
#ifndef DEBUG // this needs to be reset in the test, so it's a global variable instead
        static unsigned int fullResetCount = 0;
#endif

        printf("fullResetCount: %zu\n", fullResetCount);
        int delay = 0;

        {
            std::lock_guard<std::mutex> mutexForMapLock(mutexForMap);

            if (fileMapValue.fullResetCheckNumber < fullResetCount)
            {
                fileMapValue.position = 0;
                fileMapValue.fullResetCheckNumber = fullResetCount;
                stdcout("this delay sequence reset due to prior full reset\n");
            }

            if (fileMapValue.resetAll)
            {
                if (fullResetCount == UINT_MAX) // this probably won't ever happen
                {
                    fullResetCount = 0;

                    for (auto& it : fileMap)
                    {
                        it.second.fullResetCheckNumber = 0;
                    }

                    stdcout("fullResetCount reset\n");
                }

                fullResetCount++;
                printf("fullResetCount set to %zu, all sequences will be reset\n", fullResetCount);
            }
            else if (fileMapValue.position == fileMapValue.delays.size())
            {
                if (fileMapValue.reset)
                {
                    fileMapValue.position = 0;
                    stdcout("this delay sequence reset\n");
                }
                else
                {
                    stdcout("delay sequence already finished\n");
                }
            }

            if (fileMapValue.position < fileMapValue.delays.size())
            {
                delay = fileMapValue.delays.at(fileMapValue.position);
                fileMapValue.position++;
            }

            printf("delay is %d millisecond(s)\n\n", delay);
        }

        if (delay > 0)
        {
#ifndef DEBUG
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
#endif
        }
    }
};
