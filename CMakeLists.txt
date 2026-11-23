cmake_minimum_required(VERSION 3.16)
project(openTIT LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB_RECURSE OPENTIT_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(opentit ${OPENTIT_SOURCES})
target_include_directories(opentit PRIVATE ${CMAKE_SOURCE_DIR}/src)

find_package(Threads REQUIRED)
target_link_libraries(opentit PRIVATE Threads::Threads)

if(MSVC)
  target_compile_options(opentit PRIVATE /W4 /permissive- /utf-8)
  target_compile_definitions(opentit PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
  target_compile_options(opentit PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
