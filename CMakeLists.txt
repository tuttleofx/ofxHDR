cmake_minimum_required(VERSION 2.8) 

project(HdrTools)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
endif ()

# Check that submodule have been initialized and updated
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/openfx/include)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i\n")
endif()

# Add openfx subdirectory
add_subdirectory("${PROJECT_SOURCE_DIR}/openfx")

# Add plugin source 
add_subdirectory("${PROJECT_SOURCE_DIR}/src")

install(
  DIRECTORY scripts
  DESTINATION .
  )

