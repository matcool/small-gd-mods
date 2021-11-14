cmake_minimum_required(VERSION 3.3.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
cmake_policy(SET CMP0057 NEW)

find_file(WINDOWS_HEADER windows.h)
if(NOT WINDOWS_HEADER)
  message(FATAL_ERROR "Can't find windows.h!")
endif()
option(BUILD_SHARED_LIBS "" ON)

add_subdirectory(libraries/minhook)

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

set(COCOS2D_LIB
  ${CMAKE_SOURCE_DIR}/libraries/cocos-headers/cocos2dx/libcocos2d.lib
  ${CMAKE_SOURCE_DIR}/libraries/cocos-headers/extensions/libExtensions.lib
)

macro(setup_mod project_name file_name)
  project(${project_name})
  add_library(${project_name} SHARED ${file_name})

  if (";${ARGN};" MATCHES ";minhook;")
    target_include_directories(${project_name} PRIVATE ${MINHOOK})
    target_link_libraries(${project_name} minhook)
  endif()
  if (";${ARGN};" MATCHES ";cocos;")
    target_include_directories(${project_name} PRIVATE ${COCOS_HEADERS})
    target_link_libraries(${project_name} ${COCOS2D_LIB})
  endif()
  if (";${ARGN};" MATCHES ";gd.h;")
    target_include_directories(${project_name} PRIVATE ${GD.H})
  endif()
  if (";${ARGN};" MATCHES ";nomerge;")
    target_link_options(${project_name} PRIVATE "/OPT:REF,NOICF")
  endif()
  set_target_properties(${project_name} PROPERTIES PREFIX "")
endmacro()


setup_mod(global-clipboard src/global-clipboard.cpp
  minhook)

setup_mod(run-info src/run-info.cpp
  minhook cocos gd.h)

setup_mod(every-key-jumps src/every-key-jumps.cpp
  minhook)

setup_mod(rgb-color-input src/rgb-color-input.cpp
  minhook cocos gd.h nomerge)

setup_mod(sum-attempts src/sum-attempts.cpp
  minhook cocos gd.h nomerge)

setup_mod(ingame-screenshot src/ingame-screenshot.cpp
  minhook cocos gd.h)
target_link_libraries(ingame-screenshot opengl32)

setup_mod(song-search src/song-search.cpp
  minhook cocos gd.h)

setup_mod(fps-multiplier src/fps-multiplier.cpp
  minhook cocos)

setup_mod(menu-shaders src/menu-shaders.cpp
  minhook cocos gd.h)
target_link_libraries(menu-shaders opengl32 ${CMAKE_SOURCE_DIR}/libraries/glew32.lib ${CMAKE_SOURCE_DIR}/libraries/fmod/fmod.lib)
target_include_directories(menu-shaders PRIVATE libraries/fmod)

setup_mod(mats-orb-fix src/practice-orb-fix.cpp
  minhook cocos gd.h nomerge)

setup_mod(crash-logger src/crash-logger.cpp)
target_link_libraries(crash-logger dbghelp.lib)

setup_mod(timestamp-fix src/timestamp-fix.cpp)

