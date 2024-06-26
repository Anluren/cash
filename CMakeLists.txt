cmake_minimum_required(VERSION 3.2)

project(cash VERSION 0.1.0 LANGUAGES CXX)

set(JIT "LLVM" CACHE STRING "JIT library to use")
set(JITs "LLVM;LIBJIT;OFF")
set_property(CACHE JIT PROPERTY STRINGS ${JITs})

option(PLUGIN "Enable clang plugin." ON)

option(CODECOV "Enable code coverage." OFF)

set(DEFAULT_BUILD_TYPE "Release")

if (PLUGIN)
  find_program(CLANG clang-9 HINTS /opt/llvm-9/bin /usr/bin /usr/local/bin)
  if (NOT CLANG)
    message(FATAL_ERROR "Clang compiler is required to use the plugin")
  endif ()
  # check Clang version
  execute_process(COMMAND clang --version OUTPUT_VARIABLE clang_full_version_string )
  string(REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${clang_full_version_string})
  if (CLANG_VERSION_STRING VERSION_LESS 9.0)
    message(FATAL_ERROR "Clang version 9.0 or higher is required")
  endif ()
  set(CMAKE_C_COMPILER "clang")
  set(CMAKE_CXX_COMPILER "clang++")
endif()

# use C++17 for all projects
add_compile_options(-std=c++17)

# Must use GNUInstallDirs to install libraries into correct locations on all platforms.
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PROJECT_CONFIG ${PROJECT_NAME}Config)

option(SUPPORT_VERILOG "Build shared libraries" OFF)

if (SUPPORT_VERILOG)
  add_definitions(-DSUPPORT_VERILOG)
  # check dependent packages
  find_package(IVERILOG REQUIRED)
endif()

#
# set source files
#

set(SOURCE_FILES  
  src/core/utils.cpp
  src/core/platform.cpp  
  src/core/context.cpp
  src/core/brconv.cpp  
  src/core/lnode.cpp
  src/core/lnodeimpl.cpp
  src/core/logic.cpp
  src/core/system.cpp
  src/core/deviceimpl.cpp
  src/ast/ioimpl.cpp
  src/ast/proxyimpl.cpp
  src/ast/cdimpl.cpp
  src/ast/litimpl.cpp
  src/ast/regimpl.cpp
  src/ast/memimpl.cpp
  src/ast/selectimpl.cpp
  src/ast/opimpl.cpp
  src/ast/moduleimpl.cpp
  src/ast/timeimpl.cpp
  src/ast/assertimpl.cpp
  src/ast/printimpl.cpp
  src/ast/udfimpl.cpp    
  src/compiler/compile.cpp 
  src/compiler/simref.cpp
  src/hdl/verilogwriter.cpp
  src/hdl/firrtlwriter.cpp 
  src/sim/simulatorimpl.cpp
  src/sim/tracerimpl.cpp
  src/eda/altera/avalon_sim.cpp
)

if (JIT STREQUAL "LLVM")
  set(SOURCE_FILES ${SOURCE_FILES} 
      src/compiler/simjit.cpp 
      src/compiler/llvmjit.cpp
  )
elseif (JIT STREQUAL "LIBJIT")
  set(SOURCE_FILES ${SOURCE_FILES} 
      src/compiler/simjit.cpp 
      src/compiler/libjit.cpp
  )      
endif()

# build library
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR})

target_compile_options(${PROJECT_NAME} PRIVATE -pedantic -Werror -Wall -Wextra)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>    
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ast 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/compiler 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/sim
            ${CMAKE_CURRENT_SOURCE_DIR}/src/hdl
            ${CMAKE_CURRENT_SOURCE_DIR}/src/eda)

if (JIT STREQUAL "LIBJIT")
  message(STATUS "using LIBJIT library.")
  find_library(LIBJIT jit)
  find_path(LIBJIT_INC jit/jit.h)
  include_directories(${LIBJIT_INC})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBJIT})
  add_definitions(-DLIBJIT)
elseif (JIT STREQUAL "LLVM")
  message(STATUS "using LLVM library.")
  find_package(LLVM 9 REQUIRED)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
  include_directories(${LLVM_INCLUDE_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
  llvm_map_components_to_libnames(llvm_libs mcjit codegen native)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${llvm_libs})
  add_definitions(-DLLVMJIT)
else()
  message(STATUS "JIT disabled.")
endif()

if (CODECOV)
  message(STATUS "Code coverage is enabled")
  target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
  target_link_libraries(${PROJECT_NAME} PRIVATE --coverage)
else()
  message(STATUS "Code coverage disabled.")
endif()

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS ${PROJECT_NAME} 
  EXPORT ${PROJECT_CONFIG}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# populate public include directory
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

# This makes the project importable from the install directory
install(EXPORT ${PROJECT_CONFIG} DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# This makes the project importable from the build directory
export(TARGETS ${PROJECT_NAME} FILE ${PROJECT_CONFIG}.cmake)

# sub-projects
if (PLUGIN)
  add_subdirectory(clang)  
endif()
enable_testing()
add_subdirectory(examples)
add_subdirectory(tests)