#include <mutex>
#include <memory>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <climits>
#include "robin_hood.h"
// thank you for making this hash map, martinus.
// here's his github: https://github.com/martinus/robin-hood-hashing

#ifdef _WIN32
using wstring_or_string = std::wstring; // file paths are UTF-16LE on Windows
#else
using wstring_or_string = std::string;
#endif

#include "other_functions.h"

static void print_rh_map(robin_hood::unordered_node_map<wstring_or_string, std::unique_ptr<int[]>>& rh_map)
{
    printf("\n---------- hash map current state ----------\n");
    for (auto& it : rh_map)
    {
#ifdef _WIN32
        printf("%ls /", it.first.c_str());
#else
        printf("%s /", it.first.c_str());
#endif
        int i = 1; // used after loop to check if -1 is at the end
        for (; it.second[i] >= 0; i++)
        {
            printf(" %d", it.second[i]);
        }
        if (it.second[i] == -1)
        {
            if (i == 1)
            {
                printf(" RESET ALL");
            }
            else
            {
                printf(" REPEAT");
            }
        }
        printf(": INDEX %d\n", it.second[0]);
    }
    printf("---------------------------------------------\n\n");
}

static void test_all_inputs()
{
#ifdef _WIN32
    wchar_t path_separator = L'\\';
#else
    char path_separator = '/';
#endif
    std::ifstream input_test_file;
    input_test_file.open("test_input.txt", std::ios::binary);
    if (input_test_file.fail())
    {
        printf("couldn't open test_input.txt\n");
        return;
    }
    std::string file_content(
        (std::istreambuf_iterator<char>(input_test_file)),
        (std::istreambuf_iterator<char>())
    );
    if (file_content.length() > 0 && file_content[file_content.length() - 1] == '\n')
    {
        file_content.erase(file_content.length() - 1);
    }
    input_test_file.close();
#ifdef _WIN32
    if (file_content.length() < 2)
    {
        printf("test_input.txt byte order mark is missing\nsave test_input.txt as UTF-16 LE\n");
        return;
    }
    size_t start_index = 2; // first two bytes are BOM bytes
#else
    size_t start_index = 0;
#endif
    size_t stop_index = 0;
    robin_hood::unordered_node_map<wstring_or_string, std::unique_ptr<int[]>> rh_map;
    std::mutex delay_array_mutex = read_delays_file(rh_map);
    print_rh_map(rh_map);
    do // go through each line
    {
#ifdef _WIN32
        stop_index = file_content.find("\n\0", start_index);
        std::wstring test_path = L"";
        if (stop_index != std::string::npos)
        {
            // - 1 because of carriage return character
            test_path = string_to_wstring(file_content.substr(start_index, stop_index - start_index - 1));
        }
        else if (start_index < file_content.length())
        {
            test_path = string_to_wstring(file_content.substr(start_index));
        }
        start_index = stop_index + 2; // + 2 to go past newline character and null character
        printf("testing path: %ls\n", test_path.c_str());
#else
        stop_index = file_content.find("\n", start_index);
        std::string test_path = "";
        if (stop_index != std::string::npos)
        {
            test_path = file_content.substr(start_index, stop_index - start_index);
        }
        else if (start_index < file_content.length())
        {
            test_path = file_content.substr(start_index);
        }
        start_index = stop_index + 1; // + 1 to go past newline character
        printf("testing path: %s\n", test_path.c_str());
#endif
        // +1 so separator isn't included
        // If separator isn't found, the whole wstring is checked because npos + 1 is 0
        const wstring_or_string& test_file = &test_path[test_path.rfind(path_separator) + 1];
        delay_file(rh_map, test_file, delay_array_mutex);
        print_rh_map(rh_map);
        test_path.clear();
    }
    while (stop_index != std::string::npos);
}

int main(int argc, char* argv[])
{
    printf("\ntest start\n");
    test_all_inputs();
    printf("test finished, press Enter to exit\n");
    std::string x;
    std::getline(std::cin, x);
    return 0;
}

