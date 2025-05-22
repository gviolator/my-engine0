set(ThirdPartyFolderName "${MyEngineFolder}/3rdparty")

set (JSON_USE_EXCEPTION OFF)
set (JSONCPP_WITH_TESTS OFF)

if (NOT BUILD_SHARED_LIBS)
  set(JSONCPP_STATIC_WINDOWS_RUNTIME ON)
else()
  set(JSONCPP_STATIC_WINDOWS_RUNTIME OFF)
endif()

add_subdirectory(jsoncpp)
add_subdirectory(ModifiedSonyMath)
add_subdirectory(lua)

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
if (MYENGINE_TESTS)
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

