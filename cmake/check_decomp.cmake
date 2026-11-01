# Build-time check: the DECOMP() functions in DECOMP_DIR must be exactly the
# ones the generated code (GENERATED_DIR/decomp.list) left out. Otherwise the
# link fails with duplicate or missing f_XXXXXXXX symbols.
#   cmake -DDECOMP_DIR=... -DGENERATED_DIR=... -P check_decomp.cmake
set(have "")
set(srcs "")
if(DECOMP_DIR)
  file(GLOB_RECURSE srcs "${DECOMP_DIR}/*.cpp" "${DECOMP_DIR}/*.cc")
endif()
foreach(src IN LISTS srcs)
  file(STRINGS "${src}" lines REGEX "^DECOMP\\([0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]")
  foreach(l IN LISTS lines)
    string(SUBSTRING "${l}" 7 8 a)
    string(TOUPPER "${a}" a)
    list(APPEND have "${a}")
  endforeach()
endforeach()
list(SORT have)

set(want "")
if(EXISTS "${GENERATED_DIR}/decomp.list")
  file(STRINGS "${GENERATED_DIR}/decomp.list" want)
endif()
list(SORT want)

if(NOT have STREQUAL want)
  message(FATAL_ERROR
    "Decompiled functions changed since the code was generated.\n"
    "  decomp sources: ${have}\n"
    "  generated code: ${want}\n"
    "Re-run: opentit-recomp recompile <EBOOT.elf> -o \"${GENERATED_DIR}\" --decomp \"${DECOMP_DIR}\"")
endif()
