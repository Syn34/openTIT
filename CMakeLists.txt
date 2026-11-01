cmake_minimum_required(VERSION 3.20)
project(openTIT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(OPENTIT_BUILD_TESTS "Build the end-to-end recompiler tests" ON)
set(OPENTIT_GENERATED_DIR "" CACHE PATH
    "Folder produced by 'opentit-recomp recompile' from YOUR EBOOT.elf. When set, the game executable is built.")

if(MSVC)
  add_compile_options(/W3 /permissive- /Zc:__cplusplus /utf-8 /bigobj)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-parameter)
  # x86-64-v2 (SSSE3/SSE4.2, any x86-64 CPU since ~2009): pshufb & co. for SPU/VMX shuffles.
  option(OPENTIT_X86_64_V2 "Target x86-64-v2 on x86-64 hosts" ON)
  if(OPENTIT_X86_64_V2 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    add_compile_options(-march=x86-64-v2)
  endif()
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------- core library (loader, decoder)
add_library(opentit_core STATIC
  src/common/sha1.cpp
  src/loader/elf.cpp
  src/loader/nid.cpp
  src/loader/sfo.cpp
  src/loader/spu_images.cpp
  src/ppu/ppu_decoder.cpp
  src/ppu/ppu_disasm.cpp
  src/spu/spu_decoder.cpp
  src/spu/spu_analysis.cpp
)
target_include_directories(opentit_core PUBLIC src)

# ---------------------------------------------------------------- recompiler
add_library(opentit_recomp_lib STATIC
  src/analysis/functions.cpp
  src/recomp/emitter.cpp
  src/recomp/spu_emitter.cpp
)
target_link_libraries(opentit_recomp_lib PUBLIC opentit_core)

add_executable(opentit-recomp src/recomp/main.cpp)
target_link_libraries(opentit-recomp PRIVATE opentit_recomp_lib)

# ---------------------------------------------------------------- runtime
# OBJECT library so the self-registering HLE modules are never dropped by the linker.
add_library(opentit_runtime OBJECT
  runtime/src/boot.cpp
  runtime/src/crash.cpp
  runtime/src/dispatch.cpp
  runtime/src/hle.cpp
  runtime/src/kernel.cpp
  runtime/src/log.cpp
  runtime/src/memory.cpp
  runtime/src/syscalls.cpp
  runtime/src/threads.cpp
  runtime/src/vfs.cpp
  runtime/src/spu/spu_thread.cpp
  runtime/src/spu/lv2_spu.cpp
  runtime/src/spu/spu_dispatch.cpp
  runtime/src/hle/cellGcmSys.cpp
  runtime/src/hle/cellSysutil.cpp
  runtime/src/hle/libc.cpp
  runtime/src/hle/misc.cpp
  runtime/src/hle/cellSaveData.cpp
  runtime/src/hle/offline.cpp
  runtime/src/rsx/rsx.cpp
  runtime/src/rsx/vertex_program.cpp
  runtime/src/rsx/fragment_program.cpp
  runtime/src/rsx/shader_dump.cpp
  runtime/src/rsx/glsl.cpp
  runtime/src/rsx/glsl_compile.cpp
  runtime/src/rsx/texture.cpp
  runtime/src/rsx/geometry.cpp
  runtime/src/rsx/vk/vk_backend.cpp
  runtime/src/rsx/vk/vk_loader.cpp
  runtime/src/video.cpp
  runtime/src/input.cpp
  runtime/src/audio_out.cpp
  runtime/src/hle/sysPrxForUser.cpp
  runtime/src/hle/sys_fs.cpp
)
target_include_directories(opentit_runtime PUBLIC runtime/include src)
target_link_libraries(opentit_runtime PUBLIC opentit_core Threads::Threads)

# Shader compiler for the RSX -> GLSL -> SPIR-V path (Vulkan SDK or distro glslang).
if(DEFINED ENV{VULKAN_SDK})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}")
endif()
find_package(glslang CONFIG QUIET)
if(glslang_FOUND)
  target_link_libraries(opentit_runtime PUBLIC glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits)
  target_compile_definitions(opentit_runtime PRIVATE OPENTIT_HAS_GLSLANG)
  message(STATUS "openTIT: glslang found - shader translation enabled")
endif()
# The renderer loads vulkan-1.dll / libvulkan.so.1 at run time, so only the
# Vulkan headers are needed (Vulkan SDK, distro package, or OPENTIT_VULKAN_INCLUDE_DIR).
set(OPENTIT_VULKAN_INCLUDE_DIR "" CACHE PATH "Folder containing vulkan/vulkan.h (default: search)")
if(OPENTIT_VULKAN_INCLUDE_DIR)
  set(opentit_vk_inc "${OPENTIT_VULKAN_INCLUDE_DIR}")
else()
  find_path(opentit_vk_inc vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/Include" "$ENV{VULKAN_SDK}/include")
endif()
if(opentit_vk_inc AND EXISTS "${opentit_vk_inc}/vulkan/vulkan.h")
  target_include_directories(opentit_runtime PRIVATE "${opentit_vk_inc}")
  target_compile_definitions(opentit_runtime PRIVATE OPENTIT_HAS_VULKAN)
  target_link_libraries(opentit_runtime PUBLIC ${CMAKE_DL_LIBS})
  message(STATUS "openTIT: Vulkan headers found - RSX renderer enabled")
endif()
find_package(SDL2 CONFIG QUIET)
# On machines without SDL2 installed (typical on Windows), download it.
option(OPENTIT_FETCH_SDL2 "Download and build SDL2 if it is not installed" ON)
if(NOT SDL2_FOUND AND OPENTIT_FETCH_SDL2)
  include(FetchContent)
  message(STATUS "openTIT: SDL2 not installed - fetching it")
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(sdl2 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-2.30.9 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(sdl2)
  # SDL's own build defines SDL2::SDL2 and/or SDL2::SDL2-static.
  if(TARGET SDL2::SDL2 OR TARGET SDL2::SDL2-static)
    set(SDL2_FOUND TRUE)
  endif()
endif()
if(SDL2_FOUND)
  if(TARGET SDL2::SDL2-static)
    target_link_libraries(opentit_runtime PUBLIC SDL2::SDL2-static)
  elseif(TARGET SDL2::SDL2)
    target_link_libraries(opentit_runtime PUBLIC SDL2::SDL2)
  else()
    target_include_directories(opentit_runtime PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(opentit_runtime PUBLIC ${SDL2_LIBRARIES})
  endif()
  target_compile_definitions(opentit_runtime PRIVATE OPENTIT_HAS_SDL2 SDL_MAIN_HANDLED)
  message(STATUS "openTIT: SDL2 found - game window enabled")
endif()
if(WIN32 AND NOT SDL2_FOUND)
  # Native window, XInput and waveOut instead of SDL2.
  target_compile_definitions(opentit_runtime PRIVATE OPENTIT_HAS_WIN32_UI)
  target_link_libraries(opentit_runtime PUBLIC user32 gdi32 winmm)
  message(STATUS "openTIT: using the native Win32 window, input and audio")
endif()
if(NOT glslang_FOUND)
  message(STATUS "openTIT: glslang not found - shaders will be compiled with shaderc_shared.dll from the Vulkan SDK at run time")
endif()

# Settings for generated guest code: it is huge, so keep optimisation
# moderate and make sure FP semantics stay IEEE (no fast-math).
function(opentit_configure_generated target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /bigobj /fp:precise)
  else()
    if(MINGW)
      target_compile_options(${target} PRIVATE -Wa,-mbig-obj)
    endif()
    target_compile_options(${target} PRIVATE -fno-strict-aliasing -ffp-contract=off -Wno-unused-label
                                             -Wno-unused-variable -Wno-unused-but-set-variable)
  endif()
endfunction()

# ---------------------------------------------------------------- the game
if(OPENTIT_GENERATED_DIR)
  # Game code plus any recompiled firmware modules (gen/modules/<name>/ppu_<name>_*.cpp).
  file(GLOB_RECURSE OPENTIT_GENERATED_SOURCES CONFIGURE_DEPENDS "${OPENTIT_GENERATED_DIR}/ppu_*.cpp")
  if(NOT OPENTIT_GENERATED_SOURCES)
    message(FATAL_ERROR "No ppu_*.cpp in ${OPENTIT_GENERATED_DIR}. Run: opentit-recomp recompile <EBOOT.elf> -o <dir>")
  endif()
  # Recompiled SPU code (opentit-recomp spu-recompile ... -o <generated>/spu), optional.
  file(GLOB_RECURSE OPENTIT_GENERATED_SPU CONFIGURE_DEPENDS "${OPENTIT_GENERATED_DIR}/spu_*.cpp")
  list(APPEND OPENTIT_GENERATED_SOURCES ${OPENTIT_GENERATED_SPU})
  list(LENGTH OPENTIT_GENERATED_SOURCES n)
  message(STATUS "openTIT: building game from ${n} generated sources")
  add_executable(opentit runtime/src/main.cpp ${OPENTIT_GENERATED_SOURCES} $<TARGET_OBJECTS:opentit_runtime>)
  if(UNIX AND NOT APPLE)
    target_link_options(opentit PRIVATE -rdynamic)  # function names in crash backtraces
  endif()
  if(MINGW)
    target_link_options(opentit PRIVATE -static -static-libgcc -static-libstdc++)  # no MinGW DLLs needed
  endif()
  target_link_libraries(opentit PRIVATE opentit_runtime)
  opentit_configure_generated(opentit)
endif()

# ---------------------------------------------------------------- tests
if(OPENTIT_BUILD_TESTS)
  enable_testing()
  set(TEST_GEN_DIR "${CMAKE_BINARY_DIR}/testgen")
  add_executable(opentit-testgen tests/testgen.cpp)
  target_link_libraries(opentit-testgen PRIVATE opentit_core)

  add_custom_command(
    OUTPUT "${TEST_GEN_DIR}/test.elf" "${TEST_GEN_DIR}/ppu_0000.cpp" "${TEST_GEN_DIR}/ppu_table.cpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TEST_GEN_DIR}"
    COMMAND opentit-testgen "${TEST_GEN_DIR}/test.elf"
    COMMAND opentit-recomp recompile "${TEST_GEN_DIR}/test.elf" -o "${TEST_GEN_DIR}" --per-file 100000000
    DEPENDS opentit-testgen opentit-recomp
    COMMENT "Generating and recompiling the synthetic test ELF")

  add_executable(opentit-spu-testgen tests/spu_testgen.cpp)
  target_link_libraries(opentit-spu-testgen PRIVATE opentit_core)
  target_include_directories(opentit-spu-testgen PRIVATE tests)
  add_custom_command(
    OUTPUT "${TEST_GEN_DIR}/spu/spu_0000.cpp" "${TEST_GEN_DIR}/spu/spu_table.cpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TEST_GEN_DIR}/spu"
    COMMAND opentit-spu-testgen "${TEST_GEN_DIR}/test.spurec"
    COMMAND opentit-recomp spu-recompile "${TEST_GEN_DIR}/test.spurec" -o "${TEST_GEN_DIR}/spu"
    DEPENDS opentit-spu-testgen opentit-recomp
    COMMENT "Recompiling the SPU test programs")

  add_executable(opentit_tests
    tests/test_main.cpp
    tests/test_decoder.cpp
    tests/test_vmx.cpp
    tests/test_spu.cpp
    tests/test_lv2_spu.cpp
    tests/test_prx.cpp
    tests/test_rsx.cpp
    tests/test_rsx_cpu.cpp
    tests/test_vulkan.cpp
    tests/test_savedata.cpp
    "${TEST_GEN_DIR}/ppu_0000.cpp"
    "${TEST_GEN_DIR}/ppu_table.cpp"
    "${TEST_GEN_DIR}/spu/spu_0000.cpp"
    "${TEST_GEN_DIR}/spu/spu_table.cpp"
    tests/test_spu_recomp.cpp
    $<TARGET_OBJECTS:opentit_runtime>)
  target_link_libraries(opentit_tests PRIVATE opentit_runtime opentit_recomp_lib)
  target_compile_definitions(opentit_tests PRIVATE OPENTIT_TEST_ELF="${TEST_GEN_DIR}/test.elf")
  opentit_configure_generated(opentit_tests)
  add_test(NAME opentit_tests COMMAND opentit_tests)
endif()
