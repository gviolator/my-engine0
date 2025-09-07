set(TargetName LuaScriptModule)
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH ModuleRoot)

my_collect_files(Sources
  DIRECTORIES ${ModuleRoot}/src
  RELATIVE ${ModuleRoot}/src
  
  MASK "*.cpp" "*.c" "*.h"
  EXCLUDE
    "/platform/.*"
)


my_add_module(${TargetName}
  SOURCES ${Sources}
)

my_add_compile_options(TARGETS ${TargetName} STRICT)

target_precompile_headers(${TargetName} PRIVATE pch.h)
target_include_directories(${TargetName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(${TargetName} PUBLIC 
  LuaToolkit
)

source_group(TREE ${ModuleRoot}/src PREFIX Sources FILES ${Sources})

# install(DIRECTORY ${ModuleRoot}/include
#     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/modules/scene
#     FILES_MATCHING 
#       PATTERN "*.h"
#       PATTERN "*.hpp"
#       PATTERN "*.ipp"
# )

# install(DIRECTORY ${ModuleRoot}/include/nau/generated
#     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/modules/scene
#     FILES_MATCHING 
#       PATTERN "*.h"
#       PATTERN "*.hpp"
#       PATTERN "*.ipp"
# )

# nau_install(${TargetName} core)