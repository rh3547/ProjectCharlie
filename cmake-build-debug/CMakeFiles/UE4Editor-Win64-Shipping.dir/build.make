# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.13

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

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "C:\Program Files\JetBrains\CLion 2018.3.3\bin\cmake\win\bin\cmake.exe"

# The command to remove a file.
RM = "C:\Program Files\JetBrains\CLion 2018.3.3\bin\cmake\win\bin\cmake.exe" -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "C:\Users\reel2\Documents\Unreal Projects\ProjectCharlie"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "C:\Users\reel2\Documents\Unreal Projects\ProjectCharlie\cmake-build-debug"

# Utility rule file for UE4Editor-Win64-Shipping.

# Include the progress variables for this target.
include CMakeFiles/UE4Editor-Win64-Shipping.dir/progress.make

CMakeFiles/UE4Editor-Win64-Shipping:
	call "D:/Epic Games/UE_4.21/Engine/Build/BatchFiles/Build.bat" UE4Editor Win64 Shipping "-project=C:/Users/reel2/Documents/Unreal Projects/ProjectCharlie/ProjectCharlie.uproject" -game -progress -buildscw

UE4Editor-Win64-Shipping: CMakeFiles/UE4Editor-Win64-Shipping
UE4Editor-Win64-Shipping: CMakeFiles/UE4Editor-Win64-Shipping.dir/build.make

.PHONY : UE4Editor-Win64-Shipping

# Rule to build all files generated by this target.
CMakeFiles/UE4Editor-Win64-Shipping.dir/build: UE4Editor-Win64-Shipping

.PHONY : CMakeFiles/UE4Editor-Win64-Shipping.dir/build

CMakeFiles/UE4Editor-Win64-Shipping.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\UE4Editor-Win64-Shipping.dir\cmake_clean.cmake
.PHONY : CMakeFiles/UE4Editor-Win64-Shipping.dir/clean

CMakeFiles/UE4Editor-Win64-Shipping.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" "C:\Users\reel2\Documents\Unreal Projects\ProjectCharlie" "C:\Users\reel2\Documents\Unreal Projects\ProjectCharlie" "C:\Users\reel2\Documents\Unreal Projects\ProjectCharlie\cmake-build-debug" "C:\Users\reel2\Documents\Unreal Projects\ProjectCharlie\cmake-build-debug" "C:\Users\reel2\Documents\Unreal Projects\ProjectCharlie\cmake-build-debug\CMakeFiles\UE4Editor-Win64-Shipping.dir\DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/UE4Editor-Win64-Shipping.dir/depend

