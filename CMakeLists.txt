cmake_minimum_required(VERSION 3.20)

find_program(CLANG   clang   NAMES clang   cc )
find_program(CLANGXX clang++ NAMES clang++ c++)
set(CMAKE_C_COMPILER   ${CLANG}   CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER ${CLANGXX} CACHE STRING "" FORCE)

project(scull LANGUAGES C CXX)

include(ExternalProject)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##########
#  LLVM  #
##########

set(LLVM_SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/llvm-project/llvm)
set(LLVM_BUILD_DIR  ${CMAKE_SOURCE_DIR}/external/llvm-project/build)
set(LLVM_LIB        ${LLVM_BUILD_DIR}/lib)
set(LLVM_INC        
  ${LLVM_BUILD_DIR}/include
  ${LLVM_SOURCE_DIR}/include
)
set(LLVM_CONFIG     ${LLVM_BUILD_DIR}/bin/llvm-config)

cmake_host_system_information(RESULT NPROC QUERY NUMBER_OF_LOGICAL_CORES)

if(NOT EXISTS ${LLVM_SOURCE_DIR}/CMakeLists.txt)
  message(STATUS "[LLVM] Syncing submodule...")
  execute_process(
    COMMAND git submodule sync --recursive
    COMMAND git submodule update --init --recursive --depth 1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()

ExternalProject_Add(llvm_project
  SOURCE_DIR       ${LLVM_SOURCE_DIR}
  BINARY_DIR       ${LLVM_BUILD_DIR}
  INSTALL_COMMAND  ""
  CMAKE_GENERATOR  Ninja
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DLLVM_TARGETS_TO_BUILD=all
    -DLLVM_ENABLE_RTTI=ON
    -DLLVM_ENABLE_EH=ON
    -DLLVM_ENABLE_TERMINFO=OFF
    -DLLVM_ENABLE_ZLIB=OFF
    -DLLVM_ENABLE_ZSTD=OFF
    -DLLVM_ENABLE_LIBXML2=OFF
    -DLLVM_INCLUDE_TESTS=OFF
    -DLLVM_INCLUDE_EXAMPLES=OFF
    -DLLVM_INCLUDE_BENCHMARKS=OFF
    -DLLVM_BUILD_LLVM_DYLIB=ON
    -DLLVM_LINK_LLVM_DYLIB=ON
    -DBUILD_SHARED_LIBS=OFF
  BUILD_COMMAND    ninja -j ${NPROC}
  BUILD_BYPRODUCTS ${LLVM_SENTINEL}
  STAMP_DIR        ${CMAKE_BINARY_DIR}/llvm-stamps
)

execute_process(COMMAND ${LLVM_CONFIG} --cxxflags
  OUTPUT_VARIABLE _LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --system-libs
  OUTPUT_VARIABLE _LLVM_SYSTEM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

separate_arguments(LLVM_CXXFLAGS    UNIX_COMMAND ${_LLVM_CXXFLAGS})
separate_arguments(LLVM_SYSTEM_LIBS UNIX_COMMAND ${_LLVM_SYSTEM_LIBS})

################
# Build Types  #
################

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(_DEBUG_FLAGS  "-g -O0 -fsanitize=address,undefined -fsanitize-address-use-after-scope -fno-omit-frame-pointer -DDEBUG -Wall -Wextra")
set(_RELEASE_FLAGS "-O2 -DNDEBUG -Wall -Werror -Wextra")

set(CMAKE_C_FLAGS_DEBUG   "${_DEBUG_FLAGS}"   CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "${_RELEASE_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG   "${_DEBUG_FLAGS}"  CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${_RELEASE_FLAGS}" CACHE STRING "" FORCE)

###########
#  core   #
###########

set(CORE_INC_DIR ${CMAKE_SOURCE_DIR}/core/include)
set(CORE_SRC_DIR ${CMAKE_SOURCE_DIR}/core/src)

file(GLOB_RECURSE CORE_C_SRCS   CONFIGURE_DEPENDS ${CORE_SRC_DIR}/*.c)
file(GLOB_RECURSE CORE_CXX_SRCS CONFIGURE_DEPENDS ${CORE_SRC_DIR}/*.cpp)

add_library(core STATIC ${CORE_C_SRCS} ${CORE_CXX_SRCS})

set_target_properties(core PROPERTIES
  C_STANDARD            23
  CXX_STANDARD          17
  CXX_STANDARD_REQUIRED ON
)

target_include_directories(core PUBLIC ${CORE_INC_DIR})
add_dependencies(core llvm_project)

target_compile_options(core PRIVATE
  $<$<COMPILE_LANGUAGE:C>:   ${CMAKE_C_FLAGS}>
  $<$<COMPILE_LANGUAGE:CXX>: ${CMAKE_CXX_FLAGS}>
)

###############
#  frontend   #
###############

set(FRONTEND_INC_DIR ${CMAKE_SOURCE_DIR}/frontend/include)
set(FRONTEND_SRC_DIR ${CMAKE_SOURCE_DIR}/frontend/src)

file(GLOB_RECURSE FRONTEND_C_SRCS   CONFIGURE_DEPENDS ${FRONTEND_SRC_DIR}/*.c)
file(GLOB_RECURSE FRONTEND_CXX_SRCS CONFIGURE_DEPENDS ${FRONTEND_SRC_DIR}/*.cpp)

add_library(frontend STATIC ${FRONTEND_C_SRCS} ${FRONTEND_CXX_SRCS})

set_target_properties(frontend PROPERTIES
  C_STANDARD            23
  CXX_STANDARD          17
  CXX_STANDARD_REQUIRED ON
)

target_include_directories(frontend PUBLIC ${FRONTEND_INC_DIR})
target_link_libraries(frontend PRIVATE core)
add_dependencies(frontend llvm_project)

target_compile_options(frontend PRIVATE
  $<$<COMPILE_LANGUAGE:C>:   ${CMAKE_C_FLAGS}>
  $<$<COMPILE_LANGUAGE:CXX>: ${CMAKE_CXX_FLAGS}>
)

###########
#  sclc   #
###########

set(SCLC_INC_DIR ${CMAKE_SOURCE_DIR}/sclc/include)
set(SCLC_SRC_DIR ${CMAKE_SOURCE_DIR}/sclc/src)

file(GLOB_RECURSE SCLC_C_SRCS   CONFIGURE_DEPENDS ${SCLC_SRC_DIR}/*.c)
file(GLOB_RECURSE SCLC_CXX_SRCS CONFIGURE_DEPENDS ${SCLC_SRC_DIR}/*.cpp)

list(REMOVE_ITEM SCLC_CXX_SRCS ${SCLC_SRC_DIR}/sclc.c)

add_library(sclc_lib STATIC ${SCLC_C_SRCS} ${SCLC_CXX_SRCS})

set_target_properties(sclc_lib PROPERTIES
  C_STANDARD            23
  CXX_STANDARD          17
  CXX_STANDARD_REQUIRED ON
)

target_include_directories(sclc_lib PUBLIC ${SCLC_INC_DIR} ${LLVM_INC})

target_compile_options(sclc_lib PRIVATE
  $<$<COMPILE_LANGUAGE:C>:   ${CMAKE_C_FLAGS}>
  $<$<COMPILE_LANGUAGE:CXX>: ${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS} -Wno-unused-parameter>
)

target_link_directories(sclc_lib PRIVATE ${LLVM_LIB})
target_link_libraries(sclc_lib PUBLIC core frontend PRIVATE LLVM ${LLVM_SYSTEM_LIBS} m)
add_dependencies(sclc_lib llvm_project)

add_executable(sclc ${SCLC_SRC_DIR}/sclc.c)

set_target_properties(sclc PROPERTIES
  C_STANDARD               23
  CXX_STANDARD             17
  CXX_STANDARD_REQUIRED    ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sclc-bin
  BUILD_RPATH              "$ORIGIN/../external/llvm-project/build/lib"
)

target_link_directories(sclc PRIVATE ${LLVM_LIB})
target_link_libraries(sclc PRIVATE sclc_lib LLVM ${LLVM_SYSTEM_LIBS} m)
add_dependencies(sclc llvm_project)

add_custom_target(do-install
  COMMAND sudo cp $<TARGET_FILE:sclc> /usr/local/bin/sclc
  DEPENDS sclc
  COMMENT "[INSTALL] $<TARGET_FILE:sclc> -> /usr/local/bin/sclc"
)

############
# Examples #
############

set(EXAMPLES_DIR ${CMAKE_SOURCE_DIR}/examples)
file(GLOB_RECURSE EXAMPLE_SRCS CONFIGURE_DEPENDS ${EXAMPLES_DIR}/*.scl)

foreach(example_src ${EXAMPLE_SRCS})
  file(RELATIVE_PATH _rel ${EXAMPLES_DIR} ${example_src})
  string(REPLACE "/" "_" _tname "${_rel}")
  string(REPLACE ".scl" "" _tname "${_tname}")

  add_custom_target(example_${_tname}
    COMMAND $<TARGET_FILE:sclc> -i ${CMAKE_SOURCE_DIR}/lib ${example_src}
    DEPENDS sclc
  )
  list(APPEND _EXAMPLE_TARGETS example_${_tname})
endforeach()

add_custom_target(examples
  DEPENDS ${_EXAMPLE_TARGETS}
)

#########
# Tests #
#########

set(TESTS_INC_DIR ${CMAKE_SOURCE_DIR}/tests/include)
set(TESTS_SRC_DIR ${CMAKE_SOURCE_DIR}/tests/src)

file(GLOB_RECURSE TESTS_C_SRCS   CONFIGURE_DEPENDS ${TESTS_SRC_DIR}/*.c)
file(GLOB_RECURSE TESTS_CXX_SRCS CONFIGURE_DEPENDS ${TESTS_SRC_DIR}/*.cpp)

add_executable(tests ${TESTS_C_SRCS} ${TESTS_CXX_SRCS})

set_target_properties(tests PROPERTIES
  C_STANDARD               23
  CXX_STANDARD             17
  CXX_STANDARD_REQUIRED    ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests-bin
  BUILD_RPATH              "$ORIGIN/../external/llvm-project/build/lib"
)

target_include_directories(tests PRIVATE
  ${TESTS_INC_DIR}
  ${LLVM_INC}
)

target_compile_options(tests PRIVATE
  $<$<COMPILE_LANGUAGE:C>:   ${CMAKE_C_FLAGS}>
  $<$<COMPILE_LANGUAGE:CXX>: ${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS} -Wno-unused-parameter>
)

target_link_directories(tests PRIVATE ${LLVM_LIB})
target_link_libraries(tests PRIVATE sclc_lib core frontend LLVM ${LLVM_SYSTEM_LIBS} m)

add_dependencies(tests llvm_project)

add_custom_target(run-tests ALL
  COMMAND $<TARGET_FILE:tests>
  DEPENDS tests
  USES_TERMINAL
)
