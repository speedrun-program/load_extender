This program delays access to files.
It calls std::this_thread::sleep_for when the injected process tries to open any file from files_and_delays.txt.
I made it to make map loads take longer in a game I speedrun.

# INSTRUCTIONS

------------------------

FOR AMNESIA TDD SPEEDRUNNERS:

------------------------

Delay the .hps files instead of .map files or .map_cache files, except for menu_bg.map.
The reason to do this is because .hps files don't trigger delays during quickloads or when opening
a map folder from the debug menu.

On each line in files_and_delays.txt, write file name and delay sequence separated by a forward slash.
Delay time is in milliseconds.
Write a dash character (this character: -) as the last part of the delay sequence if you want it to restart.
If a dash character is written as the first part of the sequence, the file will restart every delay sequence
when it's loaded.

EXAMPLE:

    game_map1.hps / 1000 / 2500
    game_map2.hps / 2000 / 3000 / 4000 / -
    game_map3.hps / -

game_map1.hps will be delayed 1 second on its 1st load and 2.5 seconds on its 2nd load.
game_map2.hps will be delay 2 seconds, 3 seconds, and 4 seconds in a cycle.
game_map3.hps will restart the other maps' sequences whenever it's loaded.

ON WINDOWS: copy files_and_delays.txt into the directory of the process you want to inject.
files_and_delays.txt should be encoded as UTF-16 LE on Windows. The files that come with the Windows
version should already be encoded as UTF-16 LE. start the process you want to inject and one of the injectors,
then enter the PID of the process you want to inject (look in the details tab in Task Manager). Use
load_extender_32.exe for 32 bit processes and load_extender_64.exe for 64 bit processes.

ON LINUX AND MAC: copy files_and_delays.txt to whatever directory you're in when you
use LD_PRELOAD or DYLD_INSERT_LIBRARIES. You can copy it to your home directory for convenience.

**REMEMBER** that you should run the 64 bit version of amnesia to get random dialogue skips.

On linux, start the program along with one of the .so files using LD_PRELOAD. Use load_extender_32.so for 32 bit
processes and load_extender_64.so for 64 bit processes. Enter the command shown below, with the .so file path and
program file path in place of '.so file path' and 'program file path'.

    LD_PRELOAD='.so file path' 'program file path'

On Mac, start the program along with one of the .dylib files using DYLD_INSERT_LIBRARIES. Use load_extender_32.dylib
for 32 bit processes and load_extender_64.dylib for 64 bit processes. Enter the command shown below, with the .dylib
file path and program file path in place of '.dylib file path' and 'program file path'.

    DYLD_INSERT_LIBRARIES='.dylib file path' DYLD_FORCE_FLAT_NAMESPACE=1 'program file path'

------------------------

THINGS NOT RELEVANT TO AMNESIA TDD SPEEDRUNNERS:

------------------------

How to include a file with leading and/or trailing whitespace:
put a forward slash as the first character on the line.

EXAMPLE:

    /     leading_and_trailing_whitespace.map     / 1000 / 2000 / 3000 / -


What to do if the directory already has a file named "files_and_delays.txt" and/or "files_and_delays0.txt" in it:
rename the file "files_and_delays0.txt"/"files_and_delays1.txt"/"files_and_delays2.txt", whichever number is the
first one available.

------------------------

HOW TO WRITE test_input.txt:
(This is for debugging)

------------------------

test_input.txt should be encoded as UTF-16 LE on Windows. The file that comes with the Windows version should
already be encoded as UTF-16 LE. On each line, write the path you want to test, then run load_extender_test.

EXAMPLE:

    game_map1.map
    random/folder/game_map2.map
    not_found.map
    random/folder/not_found2.map
