set(ThirdPartyFolderName "${MyEngineFolder}/3rdparty")

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
add_subdirectory(ModifiedSonyMath)
add_subdirectory(lua)
add_subdirectory(libuv)


if (BUILD_SHARED_LIBS)
  add_library(libuv ALIAS uv)
else()
  add_library(libuv ALIAS uv_a)
endif()


# set (3rdPartyLibDirs
#   jsoncpp
# )

# set (3rdPartyLibTargets
#   jsoncpp
# )


# foreach (libDir ${3rdPartyLibDirs})
#   add_subdirectory(${libDir})
# endforeach()

# my_add_compile_options(TARGETS ${3rdPartyLibTargets})
# set_target_properties ( ${3rdPartyLibTargets}
#   PROPERTIES
#     FOLDER "${ThirdPartyFolderName}"
# )


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
      FOLDER "${FolderName}"
  )

endif()

