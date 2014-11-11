cmake_minimum_required (VERSION 2.6)
project (Tutorial)
set(CMAKE_CXX_FLAGS "-std=c++11")

# The version number.
set (Tutorial_VERSION_MAJOR 1)
set (Tutorial_VERSION_MINOR 0)
 
# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/TutorialConfig.h.in"
  "${PROJECT_BINARY_DIR}/TutorialConfig.h"
  )
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/MathFunctions")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)
add_definitions(-DGLEW_STATIC)

set(GLFW_DIRECTORY glfw-3.0.3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/${GLFW_DIRECTORY}/include)

set(GLFW_INSTALL OFF CACHE STRING "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE STRING "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE STRING "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/${GLFW_DIRECTORY})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/${GLFW_DIRECTORY}/include)

set(GLM_DIRECTORY glm-0.9.5.3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/${GLM_DIRECTORY})

################################
# Add libraries to executables

set(BINARY_FILES glfw ${GLFW_LIBRARIES} ${FREEIMAGE_LIBRARY})


# should we use our own math functions?
option (USE_MYMATH 
        "Use tutorial provided math implementation" ON) 

# add the MathFunctions library?
#
if (USE_MYMATH)
  include_directories ("${PROJECT_SOURCE_DIR}/MathFunctions")
  add_subdirectory (MathFunctions)
  set (EXTRA_LIBS ${EXTRA_LIBS} MathFunctions)
endif (USE_MYMATH)

add_subdirectory(examples)


 
