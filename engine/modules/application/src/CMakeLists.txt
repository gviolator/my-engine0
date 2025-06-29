set(TargetName AppModule)
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH ModuleRoot)

my_collect_files(Sources
  DIRECTORIES ${ModuleRoot}/src
  RELATIVE ${ModuleRoot}/src
  
  MASK "*.cpp" "*.h"
  EXCLUDE
    "/platform/.*"
)

my_collect_files(PublicHeaders
  DIRECTORIES ${ModuleRoot}/include
  RELATIVE ${ModuleRoot}
  
  MASK "*.h"
  EXCLUDE
    "/platform/.*"
  PREPEND "../"
)


my_add_module(${TargetName}
  SOURCES ${Sources} ${PublicHeaders}
)

my_add_compile_options(TARGETS ${TargetName} STRICT)

target_precompile_headers(${TargetName} PRIVATE pch.h)
target_include_directories(${TargetName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${TargetName} PUBLIC
  ${ModuleRoot}/include
#  $<BUILD_INTERFACE:${ModuleRoot}/include/nau/generated>
#  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/scene/include>
#  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/scene/include/nau/generated>
)

# target_include_directories(${TargetName} PRIVATE
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
#   $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/scene/src>
# )

# target_include_directories(${TargetName} PUBLIC
#   $<BUILD_INTERFACE:${ModuleRoot}/include>
#   $<BUILD_INTERFACE:${ModuleRoot}/include/nau/generated>
#   $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/scene/include>
# #  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/scene/include/nau/generated>
# )

# target_link_libraries(${TargetName} PUBLIC 
#   CoreAssets
# )

source_group(TREE ${ModuleRoot}/src PREFIX Sources FILES ${Sources})
source_group(TREE ${ModuleRoot}/include PREFIX Headers FILES ${PublicHeaders})

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