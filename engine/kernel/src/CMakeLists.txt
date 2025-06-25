set(TargetName MyKernel)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH moduleRoot)

my_collect_files(Sources
  DIRECTORIES ${moduleRoot}/src
  RELATIVE ${moduleRoot}/src
  MASK "*.cpp" "*.h" "*.hpp"
  EXCLUDE "platform/.*"
)

my_collect_files(PublicHeaders
  DIRECTORIES ${moduleRoot}/include
  RELATIVE ${moduleRoot}
  MASK "*.cpp" "*.h" "*.hpp"
  EXCLUDE "platform/.*"
  PREPEND "../"
)

if (${Platform_Windows})

  my_collect_files(Sources
    DIRECTORIES ${moduleRoot}/src
    RELATIVE ${moduleRoot}/src
    INCLUDE
      "platform/windows/.*"
    MASK "*.cpp" "*.h" "*.hpp"
  )

  my_collect_files(PublicHeaders
    DIRECTORIES ${moduleRoot}/include/
    RELATIVE ${moduleRoot}
    INCLUDE
    "platform/windows/.*"
    MASK "*.h" "*.hpp"
    PREPEND "../"
  )
endif()

add_library(${TargetName} ${Sources} ${PublicHeaders})


# if (Compiler_Cl)
#   target_sources(${TargetName} PRIVATE
#     ${moduleRoot}/extras/Visualizers/nau_kernel.natjmc
#     ${moduleRoot}/extras/Visualizers/nau_kernel.natvis
#     ${moduleRoot}/extras/Visualizers/third_party_libs.natjmc
#   )
# endif()

target_precompile_headers(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)

set(ThirdPartyPublicLibs
  vectormath
  libuv
)

if (NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${TargetName} PUBLIC MY_STATIC_RUNTIME)
  list(APPEND ThirdPartyPublicLibs
    jsoncpp_static
    uv_a
  )
else()
  list(APPEND ThirdPartyPublicLibs
    jsoncpp_lib
  )
endif()

target_link_libraries(${TargetName} PUBLIC ${ThirdPartyPublicLibs})
target_compile_definitions(${TargetName} PRIVATE MY_KERNEL_BUILD)

target_include_directories(${TargetName} PUBLIC
  $<BUILD_INTERFACE:${moduleRoot}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/kernel/include>
)

# target_link_libraries(${TargetName} PUBLIC
#   EASTL
#   tinyutf8
#   fmt
#   utf8cpp
#   vectormath
#   jsoncpp
#   wyhash
#   brotli
#   lzma-9.20
#   zlib-ng
#   zstd-1.4.5
#   md5
#   fast_float
# )

if (Platform_Windows)
  target_compile_definitions(${TargetName} PUBLIC MY_PLATFORM_WINDOWS)

  # Rpcrt4.lib required by uid implmentaion
  if (CMAKE_BUILD_SHARED_LIBS)
    target_link_libraries(${TargetName} PUBLIC Rpcrt4.lib version.lib)
  else()
    target_link_libraries(${TargetName} PRIVATE Rpcrt4.lib version.lib)
  endif()
endif()

my_add_compile_options(TARGETS ${TargetName} STRICT)

set_target_properties (${TargetName} PROPERTIES FOLDER "${MyEngineFolder}")

#add_nau_folder_property(${TargetName} LIB)
source_group(TREE ${moduleRoot}/src PREFIX Sources FILES ${Sources})
source_group(TREE ${moduleRoot}/include PREFIX Headers FILES ${PublicHeaders})

# nau_install(${TargetName} core)

# install(DIRECTORY ${moduleRoot}/include
#     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/kernel
#     FILES_MATCHING 
#       PATTERN "*.h"
#       PATTERN "*.hpp"
#       PATTERN "*.ipp"
# )
