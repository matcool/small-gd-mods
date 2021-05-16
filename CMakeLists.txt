cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(global-clipboard)
project(run-info)
project(every-key-jumps)
project(rgb-color-input)
project(sum-attempts)
project(ingame-screenshot)

find_file(WINDOWS_HEADER windows.h)
if(NOT WINDOWS_HEADER)
  message(FATAL_ERROR "Can't find windows.h!")
endif()

add_library(global-clipboard SHARED src/global-clipboard.cpp)
add_library(run-info SHARED src/run-info.cpp)
add_library(every-key-jumps SHARED src/every-key-jumps.cpp)
add_library(rgb-color-input SHARED src/rgb-color-input.cpp)
add_library(sum-attempts SHARED src/sum-attempts.cpp)
add_library(ingame-screenshot SHARED src/ingame-screenshot.cpp)

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
target_include_directories(rgb-color-input PRIVATE
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h
  ${CAPPUCCINO_DIR}/incl/
  ${CAPPUCCINO_DIR}/incl/cocos2dx/
  ${CAPPUCCINO_DIR}/incl/cocos2dx/include
  ${CAPPUCCINO_DIR}/incl/cocos2dx/kazmath/include
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/win32
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/third_party/win32
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/third_party/win32/OGLES
)
target_include_directories(sum-attempts PRIVATE
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h
  ${CAPPUCCINO_DIR}/incl/
  ${CAPPUCCINO_DIR}/incl/cocos2dx/
  ${CAPPUCCINO_DIR}/incl/cocos2dx/include
  ${CAPPUCCINO_DIR}/incl/cocos2dx/kazmath/include
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/win32
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/third_party/win32
  ${CAPPUCCINO_DIR}/incl/cocos2dx/platform/third_party/win32/OGLES
)
target_include_directories(ingame-screenshot PRIVATE
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h
  libraries/cocos-headers/cocos2dx/
  libraries/cocos-headers/cocos2dx/include
  libraries/cocos-headers/cocos2dx/kazmath/include
  libraries/cocos-headers/cocos2dx/platform/win32
  libraries/cocos-headers/cocos2dx/platform/third_party/win32
  libraries/cocos-headers/cocos2dx/platform/third_party/win32/OGLES
)

add_subdirectory(libraries/minhook)
target_link_libraries(global-clipboard minhook)
target_link_libraries(run-info minhook)
target_link_libraries(every-key-jumps minhook)
target_link_libraries(rgb-color-input minhook)
target_link_libraries(sum-attempts minhook)
target_link_libraries(ingame-screenshot minhook)

target_link_libraries(run-info ${CAPPUCCINO_DIR}/lib/cappuccino.lib)
target_link_libraries(rgb-color-input ${CAPPUCCINO_DIR}/lib/cappuccino.lib)
target_link_libraries(rgb-color-input ${CAPPUCCINO_DIR}/lib/libExtensions.lib)
target_link_libraries(sum-attempts ${CAPPUCCINO_DIR}/lib/cappuccino.lib)
target_link_libraries(ingame-screenshot ${CMAKE_SOURCE_DIR}/libraries/cocos-headers/cocos2dx/libcocos2d.lib)

target_link_options(rgb-color-input PRIVATE "/OPT:REF,NOICF")
target_link_options(sum-attempts PRIVATE "/OPT:REF,NOICF")

# set_target_properties(global-clipboard PROPERTIES PREFIX "")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})