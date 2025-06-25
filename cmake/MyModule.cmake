define_property(TARGET
    PROPERTY MY_MODULES_LIST
    BRIEF_DOCS "Modules list"
    FULL_DOCS "All used modules names"
)

define_property(TARGET
    PROPERTY MY_MODULES_LINKED_TARGETS
    BRIEF_DOCS "Linked modules"
    FULL_DOCS "All modules (targets) that must be linked in"
)

define_property(TARGET
    PROPERTY MY_MODULE_INTERFACE_TARGET
    BRIEF_DOCS "Module API Target"
    FULL_DOCS "Module API Target"
)


##
##
macro(my_write_static_modules_initialization resVariable)
    if (${BUILD_SHARED_LIBS})
        message(FATAL_ERROR "Static runtime configuration expected")
    endif ()

    set(GEN_PATH ${CMAKE_CURRENT_BINARY_DIR}/generated_static_modules_initialization.cpp)

    set(entryContent)
    string(APPEND entryContent "// clang-format off\n")
    string(APPEND entryContent "// automatically generated code, do not manually modify\n")
    string(APPEND entryContent "//\n")
    string(APPEND entryContent "// #my_engine_source_file\n")
    string(APPEND entryContent "//\n\n")

    string(APPEND entryContent "#include \"my/module/module.h\"\n")
    string(APPEND entryContent "#include \"my/module/module_manager.h\"\n\n")
    
    foreach (module ${ARGN})
        string(APPEND entryContent "extern my::ModulePtr createModule_${module}()\;\n")
    endforeach ()

    string(APPEND entryContent "\n\nnamespace my::module_detail\n")
    string(APPEND entryContent "{\n")
    string(APPEND entryContent "  void initializeAllStaticModules(my::IModuleManager& manager)\n  {\n")

    foreach (module ${ARGN})
        string(APPEND entryContent "    manager.registerModule(\"${module}\", createModule_${module}())\;\n")
    endforeach ()

    string(APPEND entryContent "  }\n\n")
    string(APPEND entryContent "} //namespace my::module_detail \n\n")

    string(APPEND entryContent "// clang-format on\n")

    file(WRITE ${GEN_PATH}.tmp ${entryContent})
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEN_PATH}.tmp ${GEN_PATH})

    set(${resVariable} ${GEN_PATH})
endmacro()


##
##
macro(my_generate_module_config_file targetName)
    message(NOTICE "my_generate_module_config_file ${targetName}")
    string(TOUPPER ${targetName} moduleUpper)

    set(GEN_PATH ${CMAKE_CURRENT_BINARY_DIR}/generated_${targetName}_config.h)

    set(entryContent)
    string(APPEND entryContent "// clang-format off\n")
    string(APPEND entryContent "// automatically generated code, do not manually modify\n")
    string(APPEND entryContent "//\n")
    string(APPEND entryContent "// #my_engine_source_file\n")
    string(APPEND entryContent "//\n\n")
    string(APPEND entryContent "#pragma once\n\n")

    string(APPEND entryContent "#ifdef MY_${moduleUpper}_EXPORT\n")
    string(APPEND entryContent "#undef MY_${moduleUpper}_EXPORT\n")
    string(APPEND entryContent "#endif\n\n")

    string(APPEND entryContent "#if !defined(MY_STATIC_RUNTIME)\n")
    string(APPEND entryContent "    #ifdef _MSC_VER\n")
    string(APPEND entryContent "        #ifdef MY_${moduleUpper}_BUILD\n")
    string(APPEND entryContent "            #define MY_${moduleUpper}_EXPORT __declspec(dllexport)\n")
    string(APPEND entryContent "        #else\n")
    string(APPEND entryContent "            #define MY_${moduleUpper}_EXPORT __declspec(dllimport)\n")
    string(APPEND entryContent "        #endif\n\n")
    string(APPEND entryContent "    #else\n")
    string(APPEND entryContent "        #error Unknown Compiler/OS\n")
    string(APPEND entryContent "    #endif\n\n")
    string(APPEND entryContent "#else\n")
    string(APPEND entryContent "    #define MY_${moduleUpper}_EXPORT\n")
    string(APPEND entryContent "#endif\n\n")

    string(APPEND entryContent "// clang-format on\n\n")

    #set(GEN_PATH_TMP ${GEN_PATH}.tmp)
    #file(WRITE ${GEN_PATH_TMP} ${entryContent})
    #execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEN_PATH_TMP} ${GEN_PATH})
    #file(REMOVE ${GEN_PATH_TMP})

    file(WRITE ${GEN_PATH}.tmp ${entryContent})
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GEN_PATH}.tmp ${GEN_PATH})
    target_sources(${targetName} PRIVATE ${GEN_PATH})

    target_precompile_headers(${targetName} PUBLIC
        $<BUILD_INTERFACE:${GEN_PATH}>
        $<INSTALL_INTERFACE:generated_${targetName}_config.h>
    )
   
    target_include_directories(${targetName} PUBLIC
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/${targetName}/generated>
    )

    # target_compile_definitions(${targetName} PUBLIC
    #     "NAU_${moduleUpper}_EXPORT="
    # )

    #install(FILES ${GEN_PATH} DESTINATION "include/${targetName}/generated")

    #message(NOTICE "nau_generate_module_config_file finished")
endmacro()



macro(my_check_public_modules_target)
    if (NOT TARGET MyPublicModulesTarget)
        add_library(MyPublicModulesTarget INTERFACE)
        set_property(TARGET MyPublicModulesTarget APPEND PROPERTY EXPORT_PROPERTIES
            MY_MODULES_LIST
            MY_MODULES_LINKED_TARGETS
        )

        # install(TARGETS NauLinkedModules
        #     EXPORT NauLinkedModulesTargets
        # )

        # export(EXPORT NauLinkedModulesTargets
        #     FILE ${CMAKE_BINARY_DIR}/cmake/NauLinkedModulesTargets.cmake
        # )
    endif ()
endmacro()

# macro(my_check_modules_interfaces_target)

#     if (NOT TARGET MyModulesInterfaces)
#         add_library(MyModuleInterfaces INTERFACE)

#         #install(TARGETS NauModulesApi
#         #     EXPORT NauModulesApiTargets
#         # )

#         # export(EXPORT NauModulesApiTargets
#         #     FILE ${CMAKE_BINARY_DIR}/cmake/NauModulesApiTargets.cmake
#         # )
#     endif ()

# endmacro()


##
##
function(my_add_module ModuleName)
    message(NOTICE "my_add_module ${ModuleName}")
    set(optionalValueArgs PRIVATE ENABLE_RTTI)
    set(multiValueArgs SOURCES)
    set(singleValueArgs FOLDER INTERFACE_TARGET)

    cmake_parse_arguments(MODULE "${optionalValueArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TargetName ${ModuleName})

    add_library(${TargetName} ${MODULE_SOURCES})
    my_generate_module_config_file(${TargetName})

    target_compile_definitions(${TargetName} PRIVATE
        MY_MODULE_NAME=${TargetName}
        MY_MODULE_BUILD
    )

    my_add_compile_options(${TargetName})

    if (NOT DEFINED MODULE_FOLDER)
        set(MODULE_FOLDER "${MyEngineFolder}/modules")
    endif ()

    set_target_properties(${TargetName}
        PROPERTIES
            FOLDER ${MODULE_FOLDER}
    )

    # if module interface target is specified - then suppose that module 
    # does not exports anything
    if (MODULE_INTERFACE_TARGET)
        add_library(${MODULE_INTERFACE_TARGET} INTERFACE)

        set_target_properties(${MODULE_INTERFACE_TARGET}
            PROPERTIES
                FOLDER ${MODULE_FOLDER}
        )

        set_target_properties(${TargetName}
            PROPERTIES
                MY_MODULE_INTERFACE_TARGET ${MODULE_INTERFACE_TARGET}
        )

        target_link_libraries(${TargetName} PRIVATE
            MyKernel
            ${MODULE_INTERFACE_TARGET}
        )
    else()
        target_link_libraries(${TargetName} PUBLIC MyKernel)
    endif()


    if (NOT ${MODULE_PRIVATE})
        my_check_public_modules_target()

        if (NOT MODULE_INTERFACE_TARGET)
            set_property(TARGET MyPublicModulesTarget
                APPEND PROPERTY MY_MODULES_LINKED_TARGETS
                ${TargetName}
            )
        endif()

        set_property(TARGET MyPublicModulesTarget
            APPEND PROPERTY MY_MODULES_LIST
            ${TargetName}
        )
    endif ()

    message(NOTICE "my_add_module finished")
endfunction()



##
##
function(my_target_link_modules TargetName)

    # foreach (module ${ARGN})
    #     if (NOT TARGET ${module})
    #         message(AUTHOR_WARNING "Module (${module}) expected to be target")
    #     else ()
    #         get_target_property(moduleApiTarget ${module} NAU_MODULE_API_TARGET)
    #         if (moduleApiTarget AND TARGET ${moduleApiTarget})
    #             target_link_libraries(${TargetName} PRIVATE ${moduleApiTarget})
    #         endif ()

    #         set(moduleApiTarget)
    #     endif ()
    # endforeach ()

    set(allModules ${ARGN})
    if (${BUILD_SHARED_LIBS})
        add_dependencies(${TargetName} ${allModules})
    else ()

        my_write_static_modules_initialization(modulesInitCppPath ${allModules})
        target_sources(${TargetName} PRIVATE ${modulesInitCppPath})
        target_link_libraries(${TargetName} PRIVATE ${allModules})
    endif ()

    list(JOIN allModules "," commaSeparatedModulesList)
    target_compile_definitions(${TargetName} PRIVATE
        -DMY_MODULES_LIST="${commaSeparatedModulesList}"
    )
    set_target_properties(${TargetName}
        PROPERTIES
        MODULES_LIST "${allModules}"
    )

    #set(GEN_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/generated_modules_list_${TargetName})
    #file(WRITE ${GEN_PATH} ${linkedModules})
endfunction()


##
##
function(my_target_link_public_modules TargetName)
    if (NOT TARGET MyPublicModulesTarget)
        message(STATUS "No public modules to link")
        return()
    endif ()

    # Modules list are specified all public modules.
    # But not all modules must be linked to the target:
    # only modules that are 'exported as library' or
    # if not BUILD_SHARED_LIBS (in that case all modules must be directly linked with target).
    # All modules are accessible through MY_MODULES_LIST property
    get_property(modulesList TARGET MyPublicModulesTarget
        PROPERTY MY_MODULES_LIST
    )

    list(JOIN modulesList "," commaSeparatedModulesList)
    target_compile_definitions(${TargetName} PRIVATE
        -DMY_MODULES_LIST="${commaSeparatedModulesList}"
    )

    # Also automatically link all 'module interface/api' targets, that can be
    # associated with module (if INTERFACE_TARGET was specified for my_add_module).
    foreach (module ${modulesList})
        if (TARGET ${module})
            get_property(moduleInterfaceTarget TARGET ${module} PROPERTY MY_MODULE_INTERFACE_TARGET)
            if (moduleInterfaceTarget)
                if (TARGET ${moduleInterfaceTarget})
                    target_link_libraries(${TargetName} PRIVATE ${moduleInterfaceTarget})
                else ()
                    message(AUTHOR_WARNING "Module (${module}) are specified additional api through (${moduleInterfaceTarget}) that is not a target")
                endif ()

                set(moduleInterfaceTarget)
            endif ()
        endif ()
    endforeach ()


    # Even if BUILD_SHARED_LIBS there is can be modules
    # that are exported as libraries and should to be linked with target (or other modules).
    # Such modules are accessible through MY_MODULES_LINKED_TARGETS property
    get_property(modulesLinkedTargets
        TARGET MyPublicModulesTarget
        PROPERTY MY_MODULES_LINKED_TARGETS
    )

    foreach (linkedModuleTarget ${modulesLinkedTargets})
        # if (NOT TARGET ${linkedModuleTarget})
        #     message(AUTHOR_WARNING "Module (${linkedModuleTarget}) expected to be target")
        # else ()
        target_link_libraries(${TargetName} PRIVATE ${linkedModuleTarget})
        message(STATUS "Link with module (${linkedModuleTarget}) library.")
        #endif ()
    endforeach ()

    if (NOT BUILD_SHARED_LIBS)
        my_write_static_modules_initialization(modulesInitCppPath ${modulesList})
        target_sources(${TargetName} PRIVATE ${modulesInitCppPath})
    endif ()
endfunction()
