set(ThirdPartyFolderName "${MyEngineFolder}/3rdparty")

function(configure_json)
  set (JSON_USE_EXCEPTION OFF)
  set (JSONCPP_WITH_TESTS OFF)

  if (NOT BUILD_SHARED_LIBS)
    set(JSONCPP_STATIC_WINDOWS_RUNTIME ON)
    set(LIBUV_BUILD_SHARED OFF)

  else()
    set(JSONCPP_STATIC_WINDOWS_RUNTIME OFF)
    set(LIBUV_BUILD_SHARED ON)
  endif()
  add_subdirectory(jsoncpp)
endfunction()
configure_json()

function (configure_fmt)
  add_subdirectory(fmt)

  if (NOT BUILD_SHARED_LIBS)
    set_property(TARGET fmt PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()

  set_target_properties (fmt
    PROPERTIES
      FOLDER ${ThirdPartyFolderName}
  )

endfunction()
configure_fmt()

add_subdirectory(ModifiedSonyMath)
add_subdirectory(lua)

add_subdirectory(rpmalloc)

function (configure_uv)
  add_subdirectory(libuv)
  if (BUILD_SHARED_LIBS)
    add_library(my::libuv ALIAS uv)
  else()
    add_library(my::libuv ALIAS uv_a)

    set_property(TARGET uv_a PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set_target_properties (uv uv_a
      PROPERTIES
        FOLDER ${ThirdPartyFolderName}
    )

  endif()
endfunction()
configure_uv()

# my_add_compile_options(TARGETS ${3rdPartyLibTargets})
set_target_properties (
  jsoncpp_object jsoncpp_static lua rpmalloc vectormath
  PROPERTIES
    FOLDER ${ThirdPartyFolderName}
)


## test targets
if (MY_ENGINE_TESTS)
  set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
  set(BUILD_GTEST ON CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)

  add_subdirectory(googletest)
  my_add_compile_options(TARGETS gtest gmock ENABLE_EXCEPTIONS ENABLE_RTTI)

  set_target_properties (
    gtest gmock gtest_main gmock_main
    PROPERTIES
      FOLDER ${ThirdPartyFolderName}
  )

endif()

## benchmark targets
if (MY_ENGINE_BENCHMARK)
  set(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_ENABLE_INSTALL OFF)
  set(BENCHMARK_ENABLE_DOXYGEN OFF)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  set(BENCHMARK_USE_BUNDLED_GTEST OFF)
  
  add_subdirectory(googlebench)

  set_target_properties (
    benchmark benchmark_main 
    PROPERTIES
      FOLDER ${ThirdPartyFolderName}
  )

endif()