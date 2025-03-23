set(ThirdPartyFolderName "${MyEngineFolder}/3rdparty")

add_subdirectory(jsoncpp)
add_subdirectory(ModifiedSonyMath)

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

