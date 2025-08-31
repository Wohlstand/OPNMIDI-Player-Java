# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)
project(OPNMIDI_JniBind C CXX)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-omit-frame-pointer")

string(REGEX REPLACE "-O3" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REGEX REPLACE "-O3" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -funwind-tables")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -funwind-tables")

set(FILE_PATH_OVERRIDE "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=/builds/fdroid/fdroiddata/build/ru.wohlsoft.opnmidiplayer/")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${FILE_PATH_OVERRIDE}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${FILE_PATH_OVERRIDE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${FILE_PATH_OVERRIDE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${FILE_PATH_OVERRIDE}")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} ${FILE_PATH_OVERRIDE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${FILE_PATH_OVERRIDE}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--build-id=none")
set(ENV{CFLAGS} "${FILE_PATH_OVERRIDE}")
set(ENV{CXXFLAGS} "${FILE_PATH_OVERRIDE}")

set(libOPNMIDI_SHARED OFF)
set(libOPNMIDI_STATIC ON)
set(WITH_MIDIPLAY OFF)
set(WITH_VLC_PLUGIN OFF)
set(WITH_OLD_UTILS OFF)
set(USE_NUKED_EMULATOR ON) # Enabled on a fear and risk, it's too heavy for phones!
set(USE_GX_EMULATOR ON) # Enable use of those experimental emulators
set(USE_PMDWIN_EMULATOR ON)
add_subdirectory(${OPNMIDI_JniBind_SOURCE_DIR}/src/main/cpp)

add_library(OPNMIDI INTERFACE)
target_link_libraries(OPNMIDI INTERFACE OPNMIDI_static)

add_library( # Sets the name of the library.
    opnmidi-jni

    # Sets the library as a shared library.
    SHARED

    # Provides a relative path to your source file(s).
    # Associated headers in the same location as their source
    # file are automatically included.
    src/main/cpp/opnmidi-jni.c
)

target_link_options(OPNMIDI_static PUBLIC "-Wl,-z,max-page-size=16384")
target_link_options(OPNMIDI_static PUBLIC "-Wl,-z,common-page-size=16384")

target_link_libraries(opnmidi-jni OPNMIDI)
target_include_directories(opnmidi-jni PUBLIC src/main/cpp/include)
add_dependencies(opnmidi-jni OPNMIDI_static)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library(
    # Sets the name of the path variable.
    log-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    log
)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries(
    # Specifies the target library.
    opnmidi-jni
    # Links the target library to the log library
    # included in the NDK.
    ${log-lib}
    OpenSLES
)

# target_link_libraries( native-audio-jni android log OpenSLES)

