# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)
project(MyEngine)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

set(CMAKE_CXX_STANDARD 23)

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW) 
endif()

if(NOT DEFINED ${MyEngineFolder})
  set(MyEngineFolder "MyEngine")
endif()

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/output CACHE PATH "" FORCE)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

option(MY_ENGINE_SAMPLES "Build engine samples projects" ON)
option(MY_ENGINE_TESTS "Build engine tests projects" ON)
option(MY_ENGINE_BENCHMARK "Build engine benchmark projects" ON)

# option(NAU_RTTI "Enable rtti support" OFF)
# option(NAU_EXCEPTIONS "Enable exception support" OFF)
# option(NAU_VERBOSE_LOG "Enable verbose messages for logger" OFF)
# option(NAU_FORCE_ENABLE_SHADER_COMPILER_TOOL "Enable build for ShaderCompilerTool even if NAU_CORE_TOOLS is OFF" OFF)
# option(NAU_PACKAGE_BUILD "Enabled for packaged build" OFF)
# option(NAU_MATH_USE_DOUBLE_PRECISION "Enable double precision for math" OFF)

option(BUILD_SHARED_LIBS "Build shared libs" ON)

if (BUILD_SHARED_LIBS)
  message (STATUS "Configure for dynamic (DLL) runtime")
else()
  message (STATUS "Configure for static (monolith) runtime")
endif()


include(MyCommon)
include(MyModule)
include(platform/PlatformSetup)
include(CTest)


enable_testing()


# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  #cmake_policy(SET CMP0141 NEW)
  #set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#include(cmake/MyGitVersion.cmake)
#git_check_version()

#file (STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.nauversion NAU_VERSION_STRING)
#nau_set_version("${NAU_VERSION_STRING}")

add_subdirectory(engine)
#add_subdirectory(tools)
add_subdirectory(workshop)

if (MY_ENGINE_SAMPLES)
  my_collect_cmake_subdirectories(sampleProjects ${CMAKE_CURRENT_SOURCE_DIR}/samples)
  foreach(sample ${sampleProjects})
      message(STATUS "Sample application: (${sample})")
      add_subdirectory(samples/${sample})
  endforeach()
endif()

#nau_generate_es_targets()

# configure_file("run_tests.bat.in" "${CMAKE_SOURCE_DIR}/run_tests.bat")

# install(DIRECTORY cmake
#     DESTINATION ${CMAKE_INSTALL_PREFIX}
#     FILES_MATCHING 
#       PATTERN "*.cmake"
#       PATTERN "*.bat"
# )

# install(DIRECTORY cmake/for_build/
#     DESTINATION ${CMAKE_INSTALL_PREFIX}
# )

# install(CODE "
#   message(STATUS \"Installing USD plugins from ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins to ${CMAKE_INSTALL_PREFIX}/bin/$<CONFIG>/plugins\")
#   execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins ${CMAKE_INSTALL_PREFIX}/bin/$<CONFIG>/plugins)
#")

