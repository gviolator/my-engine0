cmake_minimum_required(VERSION 3.16)

project(lua
  VERSION 5.4.6
  HOMEPAGE_URL "https://www.lua.org/"
  LANGUAGES C)

set(TargetName lua)

my_collect_files(Sources
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  MASK "*.c" "*.h"
  EXCLUDE
    "/lua\.c"
    "/luac\.c"
)

add_library(${TargetName} ${Sources})

target_include_directories(${TargetName} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/3rdPartyLibs/lua/include>
)




if (BUILD_SHARED_LIBS)
  target_compile_definitions(${TargetName} PUBLIC
    LUA_BUILD_AS_DLL
  )

  if (Compiler_LikeCl)
    target_compile_options(${TargetName} PRIVATE 
      /MD$<$<CONFIG:Debug>:d>
    )
  endif()
else()
if (Compiler_LikeCl)
    target_compile_options(${TargetName} PRIVATE 
      /MT$<$<CONFIG:Debug>:d>
    )
  endif()
endif()


my_collect_files(HEADERS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  MASK "*.h" "*.hpp" "*.ipp"
)
