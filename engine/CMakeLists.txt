add_subdirectory(3rdparty)
add_subdirectory(kernel)
add_subdirectory(mylibs)

message(NOTICE "Building engine modules")
my_collect_cmake_subdirectories(kernelModules ${CMAKE_CURRENT_SOURCE_DIR}/modules)
foreach(module ${kernelModules})
    message(STATUS "Engine module: (${module})")
    add_subdirectory(modules/${module})
endforeach()


add_subdirectory(core_application)

#add_subdirectory(core)
