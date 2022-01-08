# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/e/github/EventServer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/e/github/EventServer

# Include any dependencies generated for this target.
include handler/CMakeFiles/handler.dir/depend.make

# Include the progress variables for this target.
include handler/CMakeFiles/handler.dir/progress.make

# Include the compile flags for this target's objects.
include handler/CMakeFiles/handler.dir/flags.make

handler/CMakeFiles/handler.dir/Handler.cc.o: handler/CMakeFiles/handler.dir/flags.make
handler/CMakeFiles/handler.dir/Handler.cc.o: handler/Handler.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/e/github/EventServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object handler/CMakeFiles/handler.dir/Handler.cc.o"
	cd /mnt/e/github/EventServer/handler && clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/handler.dir/Handler.cc.o -c /mnt/e/github/EventServer/handler/Handler.cc

handler/CMakeFiles/handler.dir/Handler.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/handler.dir/Handler.cc.i"
	cd /mnt/e/github/EventServer/handler && clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/e/github/EventServer/handler/Handler.cc > CMakeFiles/handler.dir/Handler.cc.i

handler/CMakeFiles/handler.dir/Handler.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/handler.dir/Handler.cc.s"
	cd /mnt/e/github/EventServer/handler && clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/e/github/EventServer/handler/Handler.cc -o CMakeFiles/handler.dir/Handler.cc.s

handler/CMakeFiles/handler.dir/message.cc.o: handler/CMakeFiles/handler.dir/flags.make
handler/CMakeFiles/handler.dir/message.cc.o: handler/message.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/e/github/EventServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object handler/CMakeFiles/handler.dir/message.cc.o"
	cd /mnt/e/github/EventServer/handler && clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/handler.dir/message.cc.o -c /mnt/e/github/EventServer/handler/message.cc

handler/CMakeFiles/handler.dir/message.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/handler.dir/message.cc.i"
	cd /mnt/e/github/EventServer/handler && clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/e/github/EventServer/handler/message.cc > CMakeFiles/handler.dir/message.cc.i

handler/CMakeFiles/handler.dir/message.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/handler.dir/message.cc.s"
	cd /mnt/e/github/EventServer/handler && clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/e/github/EventServer/handler/message.cc -o CMakeFiles/handler.dir/message.cc.s

# Object files for target handler
handler_OBJECTS = \
"CMakeFiles/handler.dir/Handler.cc.o" \
"CMakeFiles/handler.dir/message.cc.o"

# External object files for target handler
handler_EXTERNAL_OBJECTS =

lib/libhandler.a: handler/CMakeFiles/handler.dir/Handler.cc.o
lib/libhandler.a: handler/CMakeFiles/handler.dir/message.cc.o
lib/libhandler.a: handler/CMakeFiles/handler.dir/build.make
lib/libhandler.a: handler/CMakeFiles/handler.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/e/github/EventServer/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library ../lib/libhandler.a"
	cd /mnt/e/github/EventServer/handler && $(CMAKE_COMMAND) -P CMakeFiles/handler.dir/cmake_clean_target.cmake
	cd /mnt/e/github/EventServer/handler && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/handler.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
handler/CMakeFiles/handler.dir/build: lib/libhandler.a

.PHONY : handler/CMakeFiles/handler.dir/build

handler/CMakeFiles/handler.dir/clean:
	cd /mnt/e/github/EventServer/handler && $(CMAKE_COMMAND) -P CMakeFiles/handler.dir/cmake_clean.cmake
.PHONY : handler/CMakeFiles/handler.dir/clean

handler/CMakeFiles/handler.dir/depend:
	cd /mnt/e/github/EventServer && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/e/github/EventServer /mnt/e/github/EventServer/handler /mnt/e/github/EventServer /mnt/e/github/EventServer/handler /mnt/e/github/EventServer/handler/CMakeFiles/handler.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : handler/CMakeFiles/handler.dir/depend

