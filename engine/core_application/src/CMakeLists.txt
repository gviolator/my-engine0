set (TargetName CoreApp)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH moduleRoot)

my_collect_files(Sources
  DIRECTORIES ${moduleRoot}/src
  RELATIVE ${moduleRoot}/src
  MASK "*.cpp" "*.h"
  EXCLUDE
    "/platform/.*"
)

my_collect_files(PublicHeaders
  DIRECTORIES ${moduleRoot}/include
  RELATIVE ${moduleRoot}
  MASK "*.cpp" "*.h"
  EXCLUDE
    "/platform/.*"
  PREPEND "../"
)

add_library(${TargetName} STATIC ${Sources} ${PublicHeaders})

target_precompile_headers(${TargetName} PRIVATE pch.h)

target_include_directories(${TargetName} PUBLIC
  $<BUILD_INTERFACE:${moduleRoot}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/app_framework/include>
)

# target_include_directories(${TargetName} PRIVATE
#   $<BUILD_INTERFACE:${moduleRoot}/src>
#   $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/app_framework/src>
# )

target_compile_definitions(${TargetName} PRIVATE
  MY_COREAPP_BUILD=1
)

target_link_libraries(${TargetName} PUBLIC
    AppModule
    PlatformApp
)

my_add_compile_options(TARGETS ${TargetName})

set_target_properties (${TargetName} PROPERTIES FOLDER "${MyEngineFolder}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX Sources FILES ${Sources})
source_group(TREE ${moduleRoot} PREFIX Headers FILES ${Headers})

# nau_install(${TargetName} core)

# install(DIRECTORY ${moduleRoot}/include
#     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/app_framework
#     FILES_MATCHING 
#       PATTERN "*.h"
#       PATTERN "*.hpp"
#       PATTERN "*.ipp"
# )
