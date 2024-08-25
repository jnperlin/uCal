cmake_minimum_required(VERSION 3.5.0)
project(µCal VERSION 0.1.0 LANGUAGES C)

include(CheckSymbolExists)
include(CTest)

find_package (Python COMPONENTS Interpreter Development)
find_package(Doxygen REQUIRED dot OPTIONAL_COMPONENTS mscgen dia)
find_package(LATEX COMPONENTS PDFLATEX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_subdirectory(Unity)

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wall -pedantic -Wunused -Wmissing-prototypes)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wall -pedantic -Wunused -Wmissing-prototypes)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
endif()

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

if (FALSE) # DOXYGEN_FOUND)
  message("doxygen is found -- add doc generation")
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_MAN NO)
  set(DOXYGEN_GENERATE_LATEX YES)
  set(DOXYGEN_EXTRACT_ALL YES)
  doxygen_add_docs(
      doxygen
      ${PROJECT_SOURCE_DIR}/mainpage.dox
      ${PROJECT_SOURCE_DIR}/DoxyDoc
      ${PROJECT_SOURCE_DIR}/include/ucal
      ${PROJECT_SOURCE_DIR}/src
      ALL
      COMMENT "Generate man pages"
  )
else()
  message("doxygen not found -- no autodoc generation")
endif()



check_symbol_exists("__GLIBC__" "stdlib.h" LIBC_IS_GLIBC)

enable_testing()

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(ucal STATIC src/common.c src/gregorian.c src/julian.c src/gpsdate.c src/isoweek.c src/ntpdate.c)
if(Python_FOUND)
  target_sources(ucal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/ucal/calconst.h)
endif()
set_property(TARGET ucal PROPERTY C_STANDARD 99)
set_property(TARGET ucal PROPERTY C_STANDARD_REQUIRED ON)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  # Enable large file support on 32-bit glibc, so that we can access files with large inode numbers
  target_compile_definitions(ucal PUBLIC  "_FILE_OFFSET_BITS=64")
  # Enable 64-bit time_t on 32-bit glibc, so that time stamps remain correct beyond January 2038
  target_compile_definitions(ucal PUBLIC "_TIME_BITS=64")
  message("(ucal has enabled 64-bit 'time_t' and 'off_t')")
endif()

add_executable(test-calc tests/test-calc.c)
set_property(TARGET test-calc PROPERTY C_STANDARD 99)
set_property(TARGET test-calc PROPERTY C_STANDARD_REQUIRED ON)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  target_compile_definitions(test-calc PUBLIC "_FILE_OFFSET_BITS=64")
  target_compile_definitions(test-calc PUBLIC "_TIME_BITS=64")
  message("(test-calc has enabled 64-bit 'time_t' and 'off_t')")
endif()
target_link_libraries(test-calc ucal unity)

add_executable(test-perf tests/test-perf.c)
set_property(TARGET test-perf PROPERTY C_STANDARD 99)
set_property(TARGET test-perf PROPERTY C_STANDARD_REQUIRED ON)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  target_compile_definitions(test-perf PUBLIC "_FILE_OFFSET_BITS=64")
  target_compile_definitions(test-perf PUBLIC "_TIME_BITS=64")
  message("(test-perf has enabled 64-bit 'time_t' and 'off_t')")
endif()
target_link_libraries(test-perf ucal unity)

add_executable(test-isow tests/test-isow.c)
set_property(TARGET test-isow PROPERTY C_STANDARD 99)
set_property(TARGET test-isow PROPERTY C_STANDARD_REQUIRED ON)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  target_compile_definitions(test-isow PUBLIC "_FILE_OFFSET_BITS=64")
  target_compile_definitions(test-isow PUBLIC "_TIME_BITS=64")
  message("(test-isow has enabled 64-bit 'time_t' and 'off_t')")
endif()
target_link_libraries(test-isow ucal unity)

#add_executable(avltest avltest.c)
#target_link_libraries(avltest avltreeC unity)
#set_property(TARGET avltest PROPERTY C_STANDARD 99)
#set_property(TARGET avltest PROPERTY C_STANDARD_REQUIRED ON)

add_test(NAME ucal-test COMMAND test-calc)
add_test(NAME ucal-perf COMMAND test-perf)
