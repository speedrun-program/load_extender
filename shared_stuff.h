
#ifndef DEBUG
#define printf(...) (0)
#endif

class FileHelper
{
public:
    FileHelper(const FileHelper& fh) = delete;
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

        if (_fcheck)
        {
            std::fclose(_fcheck);
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

    bool checkIfFileExists(const char* filename)
    {
#ifdef _WIN32
        bool exists = fopen_s(&_fcheck, filename, "rb") == 0;
#else
        bool exists = (_fcheck = std::fopen(filePath, "rb"));
#endif

        if (_fcheck)
        {
            std::fclose(_fcheck);
            _fcheck = nullptr;
        }

        return exists;
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
    FILE* _fcheck = nullptr;
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

static bool makeMapValue(std::vector<int>& delaysVector, MapValue& mv, FileHelper& fh)
{
    wcharOrChar ch = '\0';
    bool textRemaining = false;
    long long int delay = 0;

    for (
        textRemaining = fh.getCharacter(ch);
        ch != '\n' && textRemaining;
        textRemaining = fh.getCharacter(ch))
    {
        if (ch >= '0' && ch <= '9')
        {
            ch = ch - '0'; // this prevents a warning message
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
                mv.resetAll = true;
            }
            else
            {
                mv.reset = true;
            }

            break;
        }
        else if (ch == '/')
        {
            delaysVector.push_back((int)delay);
            delay = 0;
        }
    }

    if (delay != 0 && !mv.reset && !mv.resetAll)
    {
        delaysVector.push_back((int)delay);
    }

    // make sure to go to end of line
    for (; ch != '\n' && textRemaining; textRemaining = fh.getCharacter(ch));

    return textRemaining;
}

static bool addMapPair(myMapType& m, strType& keyStr, std::vector<int>& delaysVector, FileHelper& fh)
{
    keyStr.clear();
    delaysVector.clear();
    MapValue mv;
    wcharOrChar ch = '\0';
    bool keepWhitespace = false;
    bool textRemaining = fh.getCharacter(ch);

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
        textRemaining = fh.getCharacter(ch);
        ch != '\n' && ch != '/' && textRemaining;
        textRemaining = fh.getCharacter(ch))
    {
        if (keepWhitespace || (ch != ' ' && ch != '\f' && ch != '\r' && ch != '\t' && ch != '\v'))
        {
            keyStr.push_back(ch);
        }
    }

    if (textRemaining && ch == '/')
    {
        textRemaining = makeMapValue(delaysVector, mv, fh);
    }

    if (!keyStr.empty() && (!delaysVector.empty() || mv.resetAll))
    {
        mv.delays = delaysVector;
        strType keyStrCopy(keyStr);
        mv.delays.shrink_to_fit();
        keyStrCopy.shrink_to_fit();
        m.emplace(std::move(keyStrCopy), std::move(mv));
    }

    return textRemaining;
}

static std::mutex setupMap(myMapType& m)
{
    try
    {
        std::wstring keyStr;
        std::vector<int> delaysVector;
        FileHelper fh;

        if (!fh.checkIfFileExists("files_and_delays0.txt"))
        {
            fh.tryToOpenFile("files_and_delays.txt");
            printf("opened files_and_delays.txt\n");
        }
        else
        {
            std::string name("files_and_delays1.txt");
            unsigned int n = 1;

            while (fh.checkIfFileExists(name.c_str()))
            {
                n++;
                name = std::string("files_and_delays" + std::to_string(n) + ".txt");
            }

            name = std::string("files_and_delays" + std::to_string(n - 1) + ".txt");
            fh.tryToOpenFile(name.c_str());
            printf("opened %s\n", name.c_str());
        }

#ifdef _WIN32
        wchar_t byteOrderMark = '\0';

        if (!fh.getCharacter(byteOrderMark))
        {
            printf(
                "files_and_delays.txt byte order mark is missing\n\
                make sure files_and_delays.txt is saved as UTF-16 LE\n"
            );
        }
        else if (byteOrderMark != 0xFEFF) // not 0xFFFE because of how wchar_t is read
        {
            printf(
                "files_and_delays.txt byte order mark isn't marked as UTF-16 LE\n\
                make sure files_and_delays.txt is saved as UTF-16 LE\n"
            );
        }
#endif

        while (addMapPair(m, keyStr, delaysVector, fh));
    }
    catch (char const* e)
    {
        char const* resolveC4101Warning = e;
        printf("%s\n", resolveC4101Warning);
        m.clear(); // clear map so failure is more obvious
    }

    return std::mutex();
}

static void delayFile(myMapType& m, MapValue& mv, std::mutex& mutexForMap)
{
#ifndef DEBUG // this needs to be reset in the test, so it's a global variable instead
    static unsigned int fullResetCount = 0;
#endif

    printf("fullResetCount: %zu\n", fullResetCount);
    int delay = 0;

    {
        std::lock_guard<std::mutex> mutexForMapLock(mutexForMap);

        if (mv.fullResetCheckNumber < fullResetCount)
        {
            mv.position = 0;
            mv.fullResetCheckNumber = fullResetCount;
            printf("this delay sequence reset due to prior full reset\n");
        }

        if (mv.resetAll)
        {
            if (fullResetCount == UINT_MAX) // this probably won't ever happen
            {
                fullResetCount = 0;

                for (auto& it : m)
                {
                    it.second.fullResetCheckNumber = 0;
                }

                printf("fullResetCount reset\n");
            }

            fullResetCount++;
            printf("fullResetCount set to %zu, all sequences will be reset\n", fullResetCount);
        }
        else if (mv.position == mv.delays.size())
        {
            if (mv.reset)
            {
                mv.position = 0;
                printf("this delay sequence reset\n");
            }
            else
            {
                printf("delay sequence already finished\n");
            }
        }

        if (mv.position < mv.delays.size())
        {
            delay = mv.delays.at(mv.position);
            mv.position++;
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
