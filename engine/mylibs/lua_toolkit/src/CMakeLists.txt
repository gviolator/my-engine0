set(TargetName LuaToolkit)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH moduleRoot)

my_collect_files(Sources
  RELATIVE ${moduleRoot}/src
  DIRECTORIES ${moduleRoot}/src
  MASK "*.cpp" "*.h"
  EXCLUDE
    "/platform/.*"
)

my_collect_files(Headers
  DIRECTORIES ${moduleRoot}/include
  RELATIVE ${moduleRoot}
  MASK "*.cpp" "*.h"
  EXCLUDE
    "/platform/.*"
  PREPEND "../"
)


add_library(${TargetName} ${Sources} ${Headers})

target_precompile_headers(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)

target_compile_definitions(${TargetName} PRIVATE
  MY_LUATOOLKIT_BUILD=1
)

target_include_directories(${TargetName} PRIVATE
  ${moduleRoot}/src
)

set_target_properties (${TargetName} PROPERTIES
  FOLDER "${MyEngineFolder}/mylibs"
)


target_include_directories(${TargetName} PUBLIC
  $<BUILD_INTERFACE:${moduleRoot}/include>
#  $<BUILD_INTERFACE:${PROG_DIR}/dagorInclude>
#  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/1stPartyLibs/${TargetName}/include>
)

target_link_libraries(${TargetName} PUBLIC
  MyKernel lua
)

my_add_compile_options(TARGETS ${TargetName} STRICT)
source_group(TREE ${moduleRoot}/src PREFIX Sources FILES ${Sources})
source_group(TREE ${moduleRoot}/include PREFIX Headers FILES ${Headers})

# install(DIRECTORY ${moduleRoot}/include
#     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/lua_toolkit
#     FILES_MATCHING 
#       PATTERN "*.h"
#       PATTERN "*.hpp"
#       PATTERN "*.ipp"
# )

# nau_install(${TargetName} core)