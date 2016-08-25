#/*M///////////////////////////////////////////////////////////////////////////////////////
#
#  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
#
#  By downloading, copying, installing or using the software you agree to this license.
#  If you do not agree to this license, do not download, install,
#  copy or use the software.
#
#                           License Agreement
#                      For InstantInterface Library
#
# The MIT License (MIT)
#
# Copyright (c) 2016 Matthieu Fraissinet-Tachet (www.matthieu-ft.com)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to use,
#  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
#  subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies
#  or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
#
#M*/


cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "-std=c++14 /EHsc")


set(PROJECT_NAME InstantInterface)
set(LibraryName InstantInterface)
project(${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#set(InstantInterface_MAJOR_VERSION 0)
#set(InstantInterface_MINOR_VERSION 1)
#set(InstantInterface_PATCH_VERSION 0)
#set(InstantInterface_VERSION
#  ${InstantInterface_MAJOR_VERSION}.${InstantInterface_MINOR_VERSION}.${InstantInterface_PATCH_VERSION})
 

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/CMake/InstantInterface)
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
  "Installation directory for CMake files")


# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()



#make sure that shared libraries are found even after installation
# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")





#find packages

find_package(Boost COMPONENTS system thread  REQUIRED)
find_package(websocketpp REQUIRED)

#### required for compiling under windows
add_definitions( -DBOOST_ALL_NO_LIB )

include_directories(  ${Boost_INCLUDE_DIR}  )
include_directories( "${WEBSOCKETPP_INCLUDE_DIR}" )
include_directories(  src )


add_library(${LibraryName} STATIC 
    src/InstantInterface/Attributes.cpp
    src/InstantInterface/Attributes.hpp
    src/InstantInterface/AttributeManagement.cpp
    src/InstantInterface/WebInterface.cpp
    src/InstantInterface/InterfaceManager.cpp
    src/json/jsoncpp.cpp)

target_link_libraries(${LibraryName} ${Boost_LIBRARIES})

add_executable(basic_interface ./src/examples/basic_interface.cpp)
#set_target_properties(basic_interface PROPERTIES DEBUG_POSTFIX _d)
target_link_libraries(basic_interface ${LibraryName})

add_executable(dynamic_configurations ./src/examples/dynamic_configurations.cpp)
#set_target_properties(dynamic_configurations PROPERTIES DEBUG_POSTFIX _d)
target_link_libraries(dynamic_configurations ${LibraryName})


##############
## TEST
##############



#TESTS

#Setup CMake to run tests
enable_testing()

#Prep ourselves for compiling boost
find_package(Boost COMPONENTS unit_test_framework REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})

#I like to keep test files in a separate source directory called test
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)

#Run through each source
foreach(testSrc ${TEST_SRCS})
        #Extract the filename without an extension (NAME_WE)
        get_filename_component(testName ${testSrc} NAME_WE)

        #Add compile target
        add_executable(${testName} ${testSrc})

        #link to Boost libraries AND your targets and dependencies
        target_link_libraries(${testName} ${Boost_LIBRARIES} ${LibraryName})

        set_target_properties(${testName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests/)

        #Finally add it to test execution -
        add_test(NAME ${testName} COMMAND ${testName})
endforeach(testSrc)








  
install(TARGETS ${LibraryName}
  # IMPORTANT: Add the foo library to the "export-set"
  EXPORT InstantInterfaceTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/InstantInterface"
    COMPONENT dev)
    
install(TARGETS basic_interface
  # IMPORTANT: Add the bar executable to the "export-set"
  EXPORT InstantInterfaceTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)
    

# Add all targets to the build-tree export set
export(TARGETS ${LibraryName}
  FILE "${PROJECT_BINARY_DIR}/InstantInterfaceTargets.cmake")
 
# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE ${LibraryName})
 
# Create the FooBarConfig.cmake and FooBarConfigVersion files
file(RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_CMAKE_DIR}"
   "${INSTALL_INCLUDE_DIR}")
# ... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}")
configure_file(InstantInterfaceConfig.cmake.in
  "${PROJECT_BINARY_DIR}/InstantInterfaceConfig.cmake" @ONLY)
# ... for the install tree
set(CONF_INCLUDE_DIRS "\${InstantInterface_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file(InstantInterfaceConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/InstantInterfaceConfig.cmake" @ONLY)
  
# ... for both
#configure_file(InstantInterfaceConfigVersion.cmake.in
#  "${PROJECT_BINARY_DIR}/InstantInterfaceConfigVersion.cmake" @ONLY)
 
# Install the FooBarConfig.cmake and FooBarConfigVersion.cmake
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/InstantInterfaceConfig.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)
#  "${PROJECT_BINARY_DIR}/InstantInterfaceConfigVersion.cmake"
 
 
# Install the export set for use with the install-tree
install(EXPORT InstantInterfaceTargets DESTINATION
  "${INSTALL_CMAKE_DIR}" COMPONENT dev)

