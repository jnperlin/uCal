# ----------------------------------------------------------------------------------------------
# µCal by J.Perlinger (perlinger@nwtime.org)
#
# To the extent possible under law, the person who associated CC0 with
# µCal has waived all copyright and related or neighboring rights
# to µCal.
#
# You should have received a copy of the CC0 legalcode along with this
# work.  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
# ----------------------------------------------------------------------------------------------
# µCal -- a small calendar component in C99
# Written anno 2024 by J.Perlinger (perlinger@nwtime.org)
#
# CMake top level file to build lib, tests and docs
# ----------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5.0)
project(µCal VERSION 0.1.1 LANGUAGES C)

include(CheckSymbolExists)
include(CTest)

find_package(Python COMPONENTS Interpreter Development)
find_package(Doxygen REQUIRED dot OPTIONAL_COMPONENTS mscgen dia)
find_package(LATEX COMPONENTS PDFLATEX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_subdirectory(Unity)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wall -Wextra -pedantic -Wunused -Wmissing-prototypes)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wall -Wextra -pedantic -Wunused -Wmissing-prototypes)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
endif()

# With Python available, add automatic calculation of some calendar constants: 
if(Python_FOUND)
  add_custom_command(
        OUTPUT
          ${CMAKE_CURRENT_SOURCE_DIR}/include/ucal/calconst.h
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/template.py
          ${CMAKE_CURRENT_SOURCE_DIR}/src/calconst.h.in
          ${CMAKE_CURRENT_SOURCE_DIR}/include/ucal/calconst.h
        DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/src/calconst.h.in
          ${CMAKE_CURRENT_SOURCE_DIR}/scripts/template.py
  )
endif()

# With Doxygen (and optionally PDFLaTeX), support creation of the documentation
# via 'doxygen' and 'doxypdf' build targets
if(DOXYGEN_FOUND)
  message("added 'doxygen' target to build")
  add_custom_target(doxygen
        COMMAND
          Doxygen::doxygen ./Doxyfile
        WORKING_DIRECTORY
          ${CMAKE_CURRENT_SOURCE_DIR} 
        )
  if(LATEX_FOUND)
    message("added 'doxypdf' target to build")
    add_custom_target(doxypdf
          COMMAND
            make -C ${CMAKE_CURRENT_SOURCE_DIR}/DoxyOut/latex
          DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/DoxyOut/latex/refman.tex
          BYPRODUCTS
            ${CMAKE_CURRENT_SOURCE_DIR}/DoxyOut/latex/refman.pdf
    )
    add_dependencies(doxypdf doxygen)
  else()
    message("pdflatex not found -- no PDF doc generation")
  endif(LATEX_FOUND)
else()
  message("doxygen not found -- no autodoc generation")
endif(DOXYGEN_FOUND)


# See if we should/can enable 64bit time_t on 32bit platforms.  Might need some
# tuning if really going embedded...
check_symbol_exists("__GLIBC__" "stdlib.h" LIBC_IS_GLIBC)
if(LIBC_IS_GLIBC AND (CMAKE_SIZEOF_VOID_P EQUAL 4))
  add_compile_definitions("_FILE_OFFSET_BITS=64")
  add_compile_definitions("_TIME_BITS=64")
  message("(build has enabled 64-bit 'time_t' and 'off_t' on 32-bit platform)")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  message("(using default sizes for 'time_t' and 'off_t' on 64-bit platform")
else()
  message("(using default sizes for 'time_t' and 'off_t' on 32-bit platform")
endif()


enable_testing()

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(ucal STATIC)
target_sources(ucal PRIVATE
  src/common.c
  src/gregorian.c
  src/julian.c
  src/gpsdate.c
  src/isoweek.c
  src/ntpdate.c
  src/tsdecode.c
  src/tzposix.c
)
# the next dependency triggers regeneration of calconst.h if python is present...
if(Python_FOUND)
  target_sources(ucal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/ucal/calconst.h)
endif()

add_executable(test-calc tests/test-calc.c)
target_link_libraries(test-calc ucal unity)

add_executable(test-perf tests/test-perf.c)
target_link_libraries(test-perf ucal unity)

add_executable(test-isow tests/test-isow.c)
target_link_libraries(test-isow ucal unity)

add_executable(test-adec tests/test-adec.c)
target_link_libraries(test-adec ucal unity)

add_executable(test-posix tests/test-tzposix.c)
target_link_libraries(test-posix ucal unity)


add_test(NAME ucal-test COMMAND test-calc)
add_test(NAME ucal-perf COMMAND test-perf)
add_test(NAME ucal-isow COMMAND test-isow)
add_test(NAME ucal-adec COMMAND test-adec)

# -*- that's all folks -*-
