Someone still needs to compile this for Mac OS.

This is a load extending program. It calls std::this_thread::sleep_for when the
injected process tries to open any specified file.

# INSTRUCTIONS

On Windows, copy files_and_delays.txt into the directory of the process you want to inject.
files_and_delays.txt should be encoded as UTF-16 LE on Windows. The files that come with the Windows
version should already be encoded as UTF-16 LE.

On Linux and Mac, copy files_and_delays.txt to whatever directory you're in when you
use LD_PRELOAD or DYLD_INSERT_LIBRARIES. You can copy it to your home directory for convenience. If you're
going to run load_extender_test, copy test_input.txt into whatever directory you're going to run it from.

On Windows, start the process you want to inject and one of the injectors, then enter the
PID of the process you want to inject (look in the details tab in Task Manager).
Use load_extender_32.exe for 32 bit processes and load_extender_64.exe for 64 bit processes.

On Linux, start the program along with one of the .so files using LD_PRELOAD.
Use load_extender_32.so for 32 bit processes and load_extender_64.so for 64 bit processes.
Enter the command shown below.
Put the .so and program file paths in place of '.so file path' and 'program file path'.

    LD_PRELOAD='.so file path' 'program file path'

On Mac, start the program along with one of the .dylib files using DYLD_INSERT_LIBRARIES.
Use load_extender_32.dylib for 32 bit processes and load_extender_64.dylib for 64 bit processes.
Enter the command shown below.
Put the .dylib and program file paths in place of '.dylib file path' and 'program file path'.

    DYLD_INSERT_LIBRARIES='.dylib file path' DYLD_FORCE_FLAT_NAMESPACE=1 'program file path'

----------------------------------

HOW TO WRITE files_and_delays.txt:

----------------------------------

AMNESIA PLAYERS: Load extend .hps files instead of .map files or .map_cache files, except for menu_bg.map.
The reason to do this is because .hps files don't trigger load extension during quickloads or when opening
a map folder from the debug menu.

files_and_delays.txt is read by the .so / .dylib / .dll (or test program if you're testing input) to
see what files you want to be involved in the load extension, and what should happen when they're opened.

On each line, write file name and delay sequence separated by a forward slash.
Delay time is in milliseconds.
Write a dash character (this character: -) as the last part of the delay sequence if you want it to restart.
If a dash character is written as the first part of the sequence, the map will restart every delay sequence
when it's loaded.

EXAMPLE:

    game_map1.map / 1000 / 2500
    game_map2.map / 2000 / 3000 / 4000 / -
    game_map3.map / -
    game_map4.map / -

game_map1.map will be delayed 1 second on its 1st load and 2.5 seconds on its 2nd load.
game_map2.map will be delay 2 seconds, 3 seconds, and 4 seconds in a cycle.
game_map3.map will restart the other maps' sequences whenever it's loaded.
game_map4.map does the same thing as game_map3.map.

----------------------------------

OTHER THINGS:

----------------------------------

How to include a file with leading and/or trailing whitespace:

put a forward slash as the first character on the line.

EXAMPLE:

    /     leading_and_trailing_whitespace.map     / 1000 / 2000 / 3000 / -


What to do if the directory already has a file named "files_and_delays.txt" in it:

rename the file "files_and_delays0.txt".
if "files_and_delays0.txt" already exists, rename it "files_and_delays1.txt".
if "files_and_delays1.txt" already exists, rename it "files_and_delays2.txt".
rename it so its name uses the first available number.

----------------------------------

HOW TO WRITE test_input.txt:
(This is for debugging, you shouldn't need to worry about this)

----------------------------------

test_input.txt is read by the test program to see what paths you want to test for when the process
tries to open a file. test_input.txt should be encoded as UTF-16 LE on Windows. The file that comes with
the Windows version should already be encoded as UTF-16 LE.
On each line, write the path you want to test.

EXAMPLE:

    game_map1.map
    random/folder/game_map2.map
    
    not_found.map
    random/folder/not_found2.map

the paths game_map1.map, random/folder/game_map2.map, an empty line, not_found.map,
and random/folder/not_found2.map will be tested.
