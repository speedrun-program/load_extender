#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
#include <iostream>
#endif

#ifdef _WIN32
static std::wstring string_to_wstring(std::string line)
{
    std::wstring ws_line;
    for (size_t i = 1; i < line.length(); i += 2)
    {
        ws_line.push_back((line[i - 1]) + (line[i] << 8));
    }
    return ws_line;
}
#endif

static void remove_start_and_end_whitespace(wstring_or_string& file_name)
{
    wstring_or_string white_space = {9, 11, 12, 13, 32}; // whitespace character values
    size_t first_not_whitespace = file_name.find_first_not_of(white_space);
    size_t last_not_whitespace = file_name.find_last_not_of(white_space);
    if (first_not_whitespace == std::string::npos)
    {
        file_name.erase(0);
    }
    else
    {
        file_name = file_name.substr(first_not_whitespace, last_not_whitespace - first_not_whitespace + 1);
    }
}

static void set_key_and_value(robin_hood::unordered_node_map<wstring_or_string, std::unique_ptr<int[]>>& rh_map, wstring_or_string& line)
{
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
    static bool too_big_delay_seen = false; // this is used so the message will only be printed once
#endif
    // 47 is forward slash, 45 is dash character, 10 is newline
    // numbers used so it will work with both string and wstring
    int current_delay = 0;
    size_t current_index = (line.length() > 0 && line[0] == 47); // start at 1 if first character is forward slash
    wstring_or_string file_name;
    std::vector<int> delay_sequence;
    delay_sequence.push_back(1); // first index keeps track of next index to get delay time from
    // get file name
    for (; current_index < line.length() && line[current_index] != 47 && line[current_index] != 10; current_index++)
    {
        file_name.push_back(line[current_index]);
    }
    if (line[0] != 47) // remove leading and trailing whitespace if first character wasn't forward slash
    {
        remove_start_and_end_whitespace(file_name);
    }
    // get delay sequence
    for (current_index++; current_index < line.length() && line[current_index] != 10; current_index++)
    {
        if (line[current_index] >= 48 && line[current_index] <= 57) // character is a number
        {
            int current_digit = line[current_index] - 48;
            // avoid going over INT_MAX
            if (((INT_MAX / 10) > current_delay) || ((INT_MAX / 10) == current_delay && (INT_MAX % 10) >= current_digit))
            {
                current_delay = ((current_delay * 10) + current_digit);
            }
            else
            {
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
                if (!too_big_delay_seen)
                {
                    too_big_delay_seen = true;
                    printf("delay time can only be %d milliseconds\n", INT_MAX);
                }
#endif
                current_delay = INT_MAX;
            }
        }
        else if (line[current_index] == 47)
        {
            delay_sequence.push_back(current_delay);
            current_delay = 0;
        }
        else if (line[current_index] == 45)
        {
            delay_sequence.push_back(-1); // sequence starts over after reaching the end
            current_delay = 0;
            break;
        }
    }
    if (current_delay != 0)
    {
        delay_sequence.push_back(current_delay);
    }
    if (delay_sequence[delay_sequence.size() - 1] != -1)
    {
        delay_sequence.push_back(-2); // sequence ends without starting over
    }
    if (delay_sequence[1] != -2) // no delay sequence written on this line
    {
        std::unique_ptr<int[]> delay_sequence_unique_ptr = std::make_unique<int[]>(delay_sequence.size());
        // copy delays into std::unique_ptr<int[]>
        for (size_t i = 0; i < delay_sequence.size() && i < INT_MAX; i++)
        {
            delay_sequence_unique_ptr[i] = delay_sequence[i];
        }
        // delay sequence can't be longer that INT_MAX because first index keeps track of next index to get delay time from
        if ((delay_sequence.size() > INT_MAX) || ((delay_sequence.size() == INT_MAX) && (delay_sequence[INT_MAX] >= 0)))
        {
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
            printf("only %d delays can be stored in a delay sequence\n", INT_MAX - 2);
#endif
            delay_sequence_unique_ptr[INT_MAX] = -2;
        }
        rh_map[wstring_or_string(file_name)] = std::move(delay_sequence_unique_ptr);
    }
}

static std::mutex read_delays_file(robin_hood::unordered_node_map<wstring_or_string, std::unique_ptr<int[]>>& rh_map)
{
    std::ifstream delays_file;
    std::string txt_file_name = "files_and_delays.txt";
    // find files_and_delays.txt file with highest number at the end
    // this is used in case the directory already has a file named files_and_delays.txt
    for (int file_number = 0; std::filesystem::exists("files_and_delays" + std::to_string(file_number) + ".txt"); file_number++)
    {
        txt_file_name = "files_and_delays" + std::to_string(file_number) + ".txt";
    }
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
    printf("opening %s\n", txt_file_name.c_str());
#endif
    delays_file.open(txt_file_name, std::ios::binary);
    if (delays_file.fail())
    {
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
        printf("couldn't open %s\n", txt_file_name.c_str());
#endif
        return std::mutex();
    }
    std::string file_content(
        (std::istreambuf_iterator<char>(delays_file)),
        (std::istreambuf_iterator<char>())
    );
    delays_file.close();
#ifdef _WIN32
    if (file_content.length() < 2)
    {
        printf("files_and_delays.txt byte order mark is missing\nsave files_and_delays.txt as UTF-16 LE\n");
        return std::mutex();
    }
    size_t start_index = 2; // first two bytes are BOM bytes
#else
    size_t start_index = 0;
#endif
    size_t stop_index = 0;
    do // go through each line
    {
#ifdef _WIN32
        stop_index = file_content.find("\n\0", start_index);
        std::wstring line = L"";
        if (stop_index != std::string::npos)
        {
            // - 1 because of carriage return character
            line = string_to_wstring(file_content.substr(start_index, stop_index - start_index - 1));
        }
        else if (start_index < file_content.length())
        {
            line = string_to_wstring(file_content.substr(start_index));
        }
        start_index = stop_index + 2;
#else
        stop_index = file_content.find("\n", start_index);
        std::string line = "";
        if (stop_index != std::string::npos)
        {
            line = file_content.substr(start_index, stop_index - start_index);
        }
        else if (start_index < file_content.length())
        {
            line = file_content.substr(start_index);
        }
        start_index = stop_index + 1;
#endif
        set_key_and_value(rh_map, line);
    }
    while (stop_index != std::string::npos);
    return std::mutex();
}

void delay_file(robin_hood::unordered_node_map<wstring_or_string, std::unique_ptr<int[]>>& rh_map, const wstring_or_string& file_name, std::mutex& delay_array_mutex)
{
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
#ifdef _WIN32
    char print_format[] = "%ls";
#else
    char print_format[] = "%s";
#endif
#endif
    robin_hood::unordered_node_map<wstring_or_string, std::unique_ptr<int[]>>::iterator rh_map_iter = rh_map.find(file_name);
    if (rh_map_iter != rh_map.end())
    {
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
        printf(print_format, file_name.c_str());
        printf(" found in hash map\n");
#endif
        int delay = 0;
        {
            std::lock_guard<std::mutex> delay_array_mutex_lock(delay_array_mutex);
            std::unique_ptr<int[]>& delay_sequence = rh_map_iter->second;
            if (delay_sequence[1] == -1)
            {
                for (auto& it : rh_map)
                {
                    it.second[0] = 1;
                }
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
                printf("all delay sequences reset\n");
#endif
            }
            else if (delay_sequence[delay_sequence[0]] == -1)
            {
                delay_sequence[0] = 1;
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
                printf(print_format, file_name.c_str());
                printf(" delay sequence reset\n");
#endif
            }
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
            else if (delay_sequence[delay_sequence[0]] == -2)
            {
                printf(print_format, file_name.c_str());
                printf(" delay sequence already finished\n");
            }
#endif
            delay = delay_sequence[delay_sequence[0]];
            delay_sequence[0] += (delay_sequence[delay_sequence[0]] >= 0); // if it's -1 or -2, it's at the end already
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
            delay *= (delay >= 0); // it might be set to -1 or -2
            printf(print_format, file_name.c_str());
            printf(" access delayed for %d second(s) and %d millisecond(s)\n", delay / 1000, delay % 1000);
#endif
        }
#ifdef THIS_IS_NOT_THE_TEST_PROGRAM
        if (delay > 0) // it might be set to -1 or -2
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
#endif
    }
#ifndef THIS_IS_NOT_THE_TEST_PROGRAM
    else
    {
        printf(print_format, file_name.c_str());
        printf(" not found in hash map\n");
    }
#endif
}

