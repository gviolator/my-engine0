
##
function (my_add_compile_options)
  set(optionalValueArgs STRICT ENABLE_RTTI ENABLE_EXCEPTIONS)
  set(oneValueArgs)
  set(multiValueArgs TARGETS)
  cmake_parse_arguments(OPTIONS "${optionalValueArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  foreach (target ${OPTIONS_TARGETS})
    get_target_property(targetType ${target} TYPE)
    if(${targetType} STREQUAL "INTERFACE_LIBRARY")
      continue()
    endif()

    if (OPTIONS_STRICT)
      target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C>:${_C_STRICT_OPTIONS}>$<$<COMPILE_LANGUAGE:CXX>:${_CPP_STRICT_OPTIONS}>)
    else()
      target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C>:${_C_OPTIONS}>$<$<COMPILE_LANGUAGE:CXX>:${_CPP_OPTIONS}>)
    endif()

    if (OPTIONS_ENABLE_RTTI)
      target_compile_options(${target} PRIVATE ${_CONFIG_RTTI_ON})
    else()
      target_compile_options(${target} PRIVATE ${_CONFIG_RTTI_OFF})
    endif()

    # if (OPTIONS_ENABLE_EXCEPTIONS)
    #   target_compile_options(${target} PRIVATE ${_CONFIG_EXCEPTIONS_ON})
    # else()
    #   message("SET EXCPT OFF")
    #   target_compile_options(${target} PRIVATE ${_CONFIG_EXCEPTIONS_OFF})
    # endif()

    target_include_directories(${target} PRIVATE ${_CPP_BASE_INCLUDES})
    target_compile_definitions(${target} PRIVATE ${_DEF_C_CPP_DEFINITIONS} MY_TARGET_NAME="${target}")
  endforeach()

endfunction()



###     my_collect_files(<variable>
###         [DIRECTORIES <directories>]
###         [RELATIVE <relative-path>]
###         [MASK <globbing-expressions>]
###         [EXCLUDE <regex-to-exclude>]
###    )
###
###  Generate a list of files from <directories> (traverse all the subdirectories) that match the <globbing-expressions> and store it into the <variable>
###  If RELATIVE flag is specified, the results will be returned as relative paths to the given path.
###  If EXCLUDE is specified, all paths that matches any <regex-to-exclude> willbe removed from result
function (my_collect_files VARIABLE) 
  set(oneValueArgs RELATIVE PREPEND)
  set(multiValueArgs DIRECTORIES EXCLUDE INCLUDE MASK)
  cmake_parse_arguments(COLLECT "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (COLLECT_EXCLUDE AND COLLECT_INCLUDE)
    message(FATAL_ERROR "my_collect_files must not specify both EXCLUDE and INCLUDE parameters")
  endif()

  set (allFiles)

  foreach (dir ${COLLECT_DIRECTORIES})
    foreach (msk ${COLLECT_MASK})
      set(globExpr "${dir}/${msk}")

      if (COLLECT_RELATIVE)
        file (GLOB_RECURSE files RELATIVE ${COLLECT_RELATIVE} ${globExpr} )
      else()
        file (GLOB_RECURSE files ${globExpr})
      endif()

      list(APPEND allFiles ${files})
    endforeach()
  endforeach()

  if (COLLECT_EXCLUDE)
    foreach (re ${COLLECT_EXCLUDE})
      list (FILTER allFiles EXCLUDE REGEX ${re})
    endforeach()
  endif(COLLECT_EXCLUDE) # COLLECT_EXCLUDE

  if (COLLECT_INCLUDE)
    foreach (re ${COLLECT_INCLUDE})
        list (FILTER allFiles INCLUDE REGEX ${re})
    endforeach()
  endif(COLLECT_INCLUDE) # COLLECT_INCLUDE

  if (COLLECT_PREPEND)
    list(TRANSFORM allFiles PREPEND "${COLLECT_PREPEND}")
  endif()


  if (${VARIABLE})
    list(APPEND ${VARIABLE} ${allFiles})
    set(${VARIABLE} ${${VARIABLE}} PARENT_SCOPE)
  else()
    set(${VARIABLE} ${allFiles} PARENT_SCOPE)
  endif()

endfunction()

##
##
function(my_collect_cmake_subdirectories VARIABLE SCAN_DIR)

  file(GLOB children RELATIVE ${SCAN_DIR} ${SCAN_DIR}/*)
  set(result "")
  foreach(child ${children})
    
    if(IS_DIRECTORY ${SCAN_DIR}/${child} AND (EXISTS ${SCAN_DIR}/${child}/CMakeLists.txt))
      list(APPEND result ${child})
    endif()
  endforeach()

  set(${VARIABLE} ${result} PARENT_SCOPE)

endfunction()