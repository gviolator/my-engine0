set(TargetName TestKernelBase)


my_collect_files(SOURCES
  DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
  MASK "*.cpp" "*.h"
)


add_executable(${TargetName} ${SOURCES})

target_precompile_headers(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)

target_include_directories(${TargetName} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TargetName} PRIVATE
  TestCommonLib
)

my_add_compile_options(TARGETS ${TargetName})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
set_target_properties (${TargetName} PROPERTIES
    FOLDER "${MyEngineFolder}/tests"
)

include(GoogleTest)
gtest_discover_tests(${TargetName} DISCOVERY_TIMEOUT 10)