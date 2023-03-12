cmake_minimum_required(VERSION 3.18)

option(USE_OSXARM "macOS arm64" OFF)
option(USE_OSXINTEL "macOS x86_64" OFF)
option(USE_IOS "ios" OFF)
option(USE_TVOS "tvos" OFF)
option(USE_ANDROID "android" OFF)
option(USE_WIN32 "Win32" OFF)

if(USE_IOS)
   set(CMAKE_SYSTEM_NAME iOS)
elseif(USE_TVOS)
   set(CMAKE_SYSTEM_NAME tvOS)
elseif(USE_ANDROID)
   set(CMAKE_SYSTEM_NAME Android)
   set(CMAKE_SYSTEM_VERSION 30)
   set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
endif()

file(READ src/serum-version.h version)
string(REGEX MATCH "SERUM_VERSION_MAJOR[ ]+([0-9]+)" _tmp ${version})
set(VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "SERUM_VERSION_MINOR[ ]+([0-9]+)" _tmp ${version})
set(VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "SERUM_VERSION_PATCH[ ]+([0-9]+)" _tmp ${version})
set(VERSION_PATCH "${CMAKE_MATCH_1}")

project(serum VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
	DESCRIPTION "Cross-platform library for decoding Serum files, a colorization format for pinball ROMs.")

if(USE_OSXARM)
   set(CMAKE_OSX_ARCHITECTURES arm64)
elseif(USE_OSXINTEL)
   set(CMAKE_OSX_ARCHITECTURES x86_64)
elseif(USE_IOS OR USE_TVOS)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 16.0)
  set(CMAKE_OSX_ARCHITECTURES arm64)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_C_FLAGS -fembed-bitcode)
  set(CMAKE_CXX_FLAGS -fembed-bitcode)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if(USE_WIN32)
   add_compile_definitions(WIN32)
endif()

set(LIBRARY_SOURCES
   src/miniz/miniz.h
   src/miniz/miniz.c
   src/serum-decode.h
   src/serum-decode.cpp
)

set(INCLUDE_DIRS
   src
)

include_directories(AFTER src)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(NOT USE_IOS AND NOT USE_TVOS)
       add_library(serum_shared SHARED ${LIBRARY_SOURCES})
       set_target_properties(serum_shared PROPERTIES
          OUTPUT_NAME "serum"
          VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${SERUM_VERSION_PATCH}
          LINK_FLAGS_RELEASE -s
       )
       target_include_directories(serum_shared PUBLIC ${INCLUDE_DIRS})
       install(TARGETS serum_shared
          LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
       )
    endif()


    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        add_library(serum_static STATIC ${LIBRARY_SOURCES})
        set_target_properties(serum_static PROPERTIES OUTPUT_NAME "serum")
        target_include_directories(serum_static PUBLIC ${INCLUDE_DIRS})
        install(TARGETS serum_static
           LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        )
        install(FILES src/serum-decode.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
    endif()

else()
    include_directories(AFTER src)
    add_library(Serum ${LIBRARY_SOURCES})
endif()