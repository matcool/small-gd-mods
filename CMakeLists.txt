cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(global-clipboard VERSION 1.0.0)
project(run-info VERSION 1.0.0)
project(every-key-jumps VERSION 1.0.0)

find_file(WINDOWS_HEADER windows.h)
if(NOT WINDOWS_HEADER)
  message(FATAL_ERROR "Can't find windows.h!")
endif()

add_library(global-clipboard SHARED src/global-clipboard.cpp)
add_library(run-info SHARED src/run-info.cpp)
add_library(every-key-jumps SHARED src/every-key-jumps.cpp)

set(CAPPUCCINO_DIR ${CMAKE_SOURCE_DIR}/libraries/cappuccino-sdk/template/default-cpp/cappuccino-sdk)

target_include_directories(global-clipboard PRIVATE
  libraries/minhook/include
)
target_include_directories(run-info PRIVATE
  libraries/minhook/include
  ${CAPPUCCINO_DIR}/incl/cocos2dx/
  ${CAPPUCCINO_DIR}/incl/cocos2dx/include
  ${CAPPUCCINO_DIR}/incl/cocos2dx/kazmath/include
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/win32
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/third_party/win32
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/third_party/win32/OGLES
)
target_include_directories(every-key-jumps PRIVATE
  libraries/minhook/include
)

add_subdirectory(libraries/minhook)
target_link_libraries(global-clipboard minhook)
target_link_libraries(run-info minhook)
target_link_libraries(every-key-jumps minhook)

target_link_libraries(run-info ${CAPPUCCINO_DIR}/lib/cappuccino.lib)

# set_target_properties(global-clipboard PROPERTIES PREFIX "")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})