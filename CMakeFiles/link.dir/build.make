# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Produce verbose output by default.
VERBOSE = 1

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/links/Code/link-server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/links/Code/link-server

# Include any dependencies generated for this target.
include CMakeFiles/link.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/link.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/link.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/link.dir/flags.make

CMakeFiles/link.dir/link/log.cc.o: CMakeFiles/link.dir/flags.make
CMakeFiles/link.dir/link/log.cc.o: link/log.cc
CMakeFiles/link.dir/link/log.cc.o: CMakeFiles/link.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/links/Code/link-server/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/link.dir/link/log.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/link.dir/link/log.cc.o -MF CMakeFiles/link.dir/link/log.cc.o.d -o CMakeFiles/link.dir/link/log.cc.o -c /home/links/Code/link-server/link/log.cc

CMakeFiles/link.dir/link/log.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/link.dir/link/log.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/links/Code/link-server/link/log.cc > CMakeFiles/link.dir/link/log.cc.i

CMakeFiles/link.dir/link/log.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/link.dir/link/log.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/links/Code/link-server/link/log.cc -o CMakeFiles/link.dir/link/log.cc.s

# Object files for target link
link_OBJECTS = \
"CMakeFiles/link.dir/link/log.cc.o"

# External object files for target link
link_EXTERNAL_OBJECTS =

lib/liblink.so: CMakeFiles/link.dir/link/log.cc.o
lib/liblink.so: CMakeFiles/link.dir/build.make
lib/liblink.so: CMakeFiles/link.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/links/Code/link-server/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library lib/liblink.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/link.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/link.dir/build: lib/liblink.so
.PHONY : CMakeFiles/link.dir/build

CMakeFiles/link.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/link.dir/cmake_clean.cmake
.PHONY : CMakeFiles/link.dir/clean

CMakeFiles/link.dir/depend:
	cd /home/links/Code/link-server && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/links/Code/link-server /home/links/Code/link-server /home/links/Code/link-server /home/links/Code/link-server /home/links/Code/link-server/CMakeFiles/link.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/link.dir/depend

