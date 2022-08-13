cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
cmake_policy(SET CMP0057 NEW)

# option(BUILD_SHARED_LIBS "" ON)


macro(setup_mod project_name file_name)
  project(${project_name})
  add_library(${project_name} SHARED ${file_name})
  set(CMAKE_SIZEOF_VOID_P 4)

  if (";${ARGN};" MATCHES ";minhook;")
    target_include_directories(${project_name} PRIVATE libraries/minhook/include)
    target_link_libraries(${project_name} minhook)
  endif()
  if (";${ARGN};" MATCHES ";cocos;")
    target_link_libraries(${project_name} cocos2d)
  endif()
  if (";${ARGN};" MATCHES ";gd.h;")
    target_include_directories(${project_name} PRIVATE libraries/gd.h libraries/gd.h/include)
  endif()
  if (";${ARGN};" MATCHES ";nomerge;")
    target_link_options(${project_name} PRIVATE "/OPT:REF,NOICF")
  endif()
  if (";${ARGN};" MATCHES ";fmod;")
    target_link_libraries(${project_name} ${CMAKE_SOURCE_DIR}/libraries/fmod/fmod.lib)
    target_include_directories(${project_name} PRIVATE libraries/fmod)
  endif()
  if (";${ARGN};" MATCHES ";mat;")
    target_include_directories(${project_name} PRIVATE libraries/mat-dash/include)
  endif()
  if (";${ARGN};" MATCHES ";modmenu;")
    target_link_libraries(${project_name} ${CMAKE_SOURCE_DIR}/libraries/gdhm-api/lib/ToastedMarshmellow.lib)
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
      target_link_options(${project_name} PRIVATE "/DELAYLOAD:ToastedMarshmellow.dll")
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
      # these actually dont set the link options (thanks cmake), so instead manually provide -Wl,
      target_link_options(${project_name} PRIVATE "-Wl,/delayload:ToastedMarshmellow.dll" "-ldelayimp")
    else()
      message("gdhm api might not work (what compiler are you using tf)")
    endif()
  endif()
  if (";${ARGN};" MATCHES ";opengl;")
    target_link_libraries(${project_name} opengl32 ${CMAKE_SOURCE_DIR}/libraries/glew32.lib)
  endif()
  set_target_properties(${project_name} PROPERTIES PREFIX "")
endmacro()

setup_mod(global-clipboard src/global-clipboard.cpp
  minhook)

setup_mod(run-info src/run-info.cpp
  minhook cocos gd.h mat modmenu)

setup_mod(every-key-jumps src/every-key-jumps.cpp
  minhook)

setup_mod(rgb-color-input src/rgb-color-input.cpp
  minhook cocos gd.h mat)

setup_mod(sum-attempts src/sum-attempts.cpp
  minhook cocos gd.h nomerge)

setup_mod(ingame-screenshot src/ingame-screenshot.cpp
  minhook cocos gd.h opengl)

setup_mod(song-search src/song-search.cpp
  minhook cocos gd.h)

setup_mod(fps-multiplier src/fps-multiplier.cpp
  minhook cocos)

setup_mod(menu-shaders src/menu-shaders.cpp
  minhook cocos gd.h fmod opengl)

setup_mod(mats-orb-fix src/practice-orb-fix.cpp
  minhook cocos gd.h nomerge)

setup_mod(crash-logger src/crash-logger.cpp)
target_link_libraries(crash-logger dbghelp.lib)

setup_mod(timestamp-fix src/timestamp-fix.cpp)

setup_mod(internal-res src/internal-res.cpp
  minhook cocos gd.h mat modmenu opengl)

setup_mod(fix-effect-lines src/fix-effect-lines.cpp)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # ensure 32 bit on clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target i386-pc-windows-msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target i386-pc-windows-msvc")
  add_definitions("--target=i386-pc-windows-msvc")
endif()

add_subdirectory(libraries/minhook)
add_subdirectory(libraries/cocos-headers)