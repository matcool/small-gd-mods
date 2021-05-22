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
project(song-search)

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
add_library(song-search SHARED src/song-search.cpp)

set(MINHOOK libraries/minhook/include)
set(COCOS_HEADERS
  libraries/cocos-headers/
  libraries/cocos-headers/extensions
  libraries/cocos-headers/cocos2dx
  libraries/cocos-headers/cocos2dx/include
  libraries/cocos-headers/cocos2dx/kazmath/include
  libraries/cocos-headers/cocos2dx/platform/win32
  libraries/cocos-headers/cocos2dx/platform/third_party/win32
  libraries/cocos-headers/cocos2dx/platform/third_party/win32/OGLES
)
set(GD.H libraries/gd.h libraries/gd.h/include)

target_include_directories(global-clipboard  PRIVATE ${MINHOOK})
target_include_directories(run-info          PRIVATE ${MINHOOK} ${COCOS_HEADERS} ${GD.H})
target_include_directories(every-key-jumps   PRIVATE ${MINHOOK})
target_include_directories(rgb-color-input   PRIVATE ${MINHOOK} ${COCOS_HEADERS} ${GD.H})
target_include_directories(sum-attempts      PRIVATE ${MINHOOK} ${COCOS_HEADERS} ${GD.H})
target_include_directories(ingame-screenshot PRIVATE ${MINHOOK} ${COCOS_HEADERS} ${GD.H})
target_include_directories(song-search       PRIVATE ${MINHOOK} ${COCOS_HEADERS} ${GD.H})

add_subdirectory(libraries/minhook)
set(COCOS2D_LIB
  ${CMAKE_SOURCE_DIR}/libraries/cocos-headers/cocos2dx/libcocos2d.lib
  ${CMAKE_SOURCE_DIR}/libraries/cocos-headers/extensions/libExtensions.lib
)

target_link_libraries(global-clipboard  minhook)
target_link_libraries(run-info          minhook ${COCOS2D_LIB})
target_link_libraries(every-key-jumps   minhook)
target_link_libraries(rgb-color-input   minhook ${COCOS2D_LIB})
target_link_libraries(sum-attempts      minhook ${COCOS2D_LIB})
target_link_libraries(ingame-screenshot minhook ${COCOS2D_LIB} opengl32)
target_link_libraries(song-search       minhook ${COCOS2D_LIB})

target_link_options(rgb-color-input PRIVATE "/OPT:REF,NOICF")
target_link_options(sum-attempts PRIVATE "/OPT:REF,NOICF")
