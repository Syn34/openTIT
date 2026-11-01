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
)
target_include_directories(opentit_core PUBLIC src)

# ---------------------------------------------------------------- recompiler
add_library(opentit_recomp_lib STATIC
  src/analysis/functions.cpp
  src/recomp/emitter.cpp
)
target_link_libraries(opentit_recomp_lib PUBLIC opentit_core)

add_executable(opentit-recomp src/recomp/main.cpp)
target_link_libraries(opentit-recomp PRIVATE opentit_recomp_lib)

# ---------------------------------------------------------------- runtime
# OBJECT library so the self-registering HLE modules are never dropped by the linker.
add_library(opentit_runtime OBJECT
  runtime/src/boot.cpp
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
  runtime/src/hle/cellGcmSys.cpp
  runtime/src/hle/cellSysutil.cpp
  runtime/src/hle/libc.cpp
  runtime/src/hle/misc.cpp
  runtime/src/rsx/rsx.cpp
  runtime/src/rsx/vertex_program.cpp
  runtime/src/rsx/fragment_program.cpp
  runtime/src/rsx/shader_dump.cpp
  runtime/src/rsx/glsl.cpp
  runtime/src/rsx/glsl_compile.cpp
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
  target_compile_definitions(opentit_runtime PUBLIC OPENTIT_HAS_GLSLANG)
  message(STATUS "openTIT: glslang found - shader translation enabled")
else()
  message(STATUS "openTIT: glslang not found - shaders cannot be compiled (install the Vulkan SDK)")
endif()

# Settings for generated guest code: it is huge, so keep optimisation
# moderate and make sure FP semantics stay IEEE (no fast-math).
function(opentit_configure_generated target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /bigobj /fp:precise)
  else()
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
  list(LENGTH OPENTIT_GENERATED_SOURCES n)
  message(STATUS "openTIT: building game from ${n} generated sources")
  add_executable(opentit runtime/src/main.cpp ${OPENTIT_GENERATED_SOURCES} $<TARGET_OBJECTS:opentit_runtime>)
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

  add_executable(opentit_tests
    tests/test_main.cpp
    tests/test_decoder.cpp
    tests/test_vmx.cpp
    tests/test_spu.cpp
    tests/test_lv2_spu.cpp
    tests/test_prx.cpp
    tests/test_rsx.cpp
    "${TEST_GEN_DIR}/ppu_0000.cpp"
    "${TEST_GEN_DIR}/ppu_table.cpp"
    $<TARGET_OBJECTS:opentit_runtime>)
  target_link_libraries(opentit_tests PRIVATE opentit_runtime opentit_recomp_lib)
  target_compile_definitions(opentit_tests PRIVATE OPENTIT_TEST_ELF="${TEST_GEN_DIR}/test.elf")
  opentit_configure_generated(opentit_tests)
  add_test(NAME opentit_tests COMMAND opentit_tests)
endif()
