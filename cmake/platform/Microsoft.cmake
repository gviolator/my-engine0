# Setup for Visual C/C++ compiler for Win64
# Compiler_LikeCl:      cl, clang-cl
# Compiler_MSVC:    cl only
# Compiler_ClangCl: clang-cl only
# Compiler_Clang:   currently not supported

set(Platform_Windows ON)
set(Platform_Name windows)

if(${Host_Arch} STREQUAL x64)
    set(Platform_Win64 ON)
elseif(${Host_Arch} STREQUAL x86)
    set(Platform_Win32 ON)
else()
    message(FATAL_ERROR "Unknown microsoft target platform:(${Platform})")
endif()

if(MSVC) # MSVC: cl.exe, clang-cl.exe
    set(Compiler_LikeCl ON)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(Compiler_MSVC ON)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(Compiler_ClangCl ON)
    else()
        message(FATAL_ERROR "Unsupported compiler:(${CMAKE_CXX_COMPILER_ID}) on (${CMAKE_SYSTEM_NAME})")
    endif()
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(Compiler_Clang ON)
    endif()
    message(FATAL_ERROR "Unsupported compiler:(${CMAKE_CXX_COMPILER_ID}). Currently supported only msvc like compilers")
endif()

#execute_process(
#    COMMAND ${CMAKE_COMMAND} --help-variable-list
#    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake_var_full_list.txt"
#)

#file(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/cmake_var_full_list.txt" VAR_FULL_LIST)
#foreach(var ${VAR_FULL_LIST})
#    if("${var}" MATCHES "<CONFIG>")
#        if("${var}" MATCHES "<LANG>")
#            foreach(lang C CXX)
#                # (supported languages list from https://cmake.org/cmake/help/latest/command/project.html)
#                string(REPLACE "<LANG>" "${lang}" lang_var "${var}")
#                list(APPEND CONFIG_VAR_LIST "${lang_var}")
#            endforeach()
#        else()
#            list(APPEND CONFIG_VAR_LIST "${var}")
#        endif()
#    endif()
#endforeach()
#unset(VAR_FULL_LIST)

#function(copy_configuration_type config_from config_to)
#    string(TOUPPER "${config_from}" config_from)
#    string(TOUPPER "${config_to}" config_to)
#    foreach(config_var ${CONFIG_VAR_LIST})
#        string(REPLACE "<CONFIG>" "${config_from}" config_var_from "${config_var}")
#        string(REPLACE "<CONFIG>" "${config_to}"   config_var_to   "${config_var}")
#        set("${config_var_to}" "${${config_var_from}}" PARENT_SCOPE)
#    endforeach()
#endfunction()

#copy_configuration_type(RELEASE RELWITHDEBINFO)

set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
#include(InstallRequiredSystemLibraries)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


set(_DEF_C_CPP_DEFINITIONS_DEBUG)
set(_CONFIG_OPTIONS_RELWITHDEBINFO
    _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
    NDEBUG=1
)
set(_DEF_C_CPP_DEFINITIONS_RELEASE
    NDEBUG=1
    _SECURE_SCL=0
    _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
)

set(_DEF_C_CPP_DEFINITIONS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:${_DEF_C_CPP_DEFINITIONS_DEBUG}>
    $<$<CONFIG:Release>:${_DEF_C_CPP_DEFINITIONS_RELEASE}>
    $<$<CONFIG:RelWithDebInfo>:${_CONFIG_OPTIONS_RELWITHDEBINFO}>
)

if(Compiler_LikeCl)
    if(NOT BUILD_SHARED_LIBS)
        set(CrtRuntime_DEBUG /MTd)
        set(CrtRuntime_RELEASE /MT)
        set(CrtRuntime_RELWITHDEBINFO /MT)
    else()
        set(CrtRuntime_DEBUG /MDd)
        set(CrtRuntime_RELEASE /MD)
        set(CrtRuntime_RELWITHDEBINFO /MD)
    endif()

    set(_DEF_C_CPP_OPTIONS
        /c
        /nologo
        /Zc:forScope
        /Zc:inline
        /Zc:wchar_t
        /J
        /JMC
        /bigobj
        $<$<CONFIG:Debug>:${CrtRuntime_DEBUG}>
        $<$<CONFIG:Release>:${CrtRuntime_RELEASE}>
        $<$<CONFIG:RelWithDebInfo>:${CrtRuntime_RELWITHDEBINFO}>
    )

    if(BUILD_DEBUG_WITH_ASAN)
        list(APPEND _DEF_C_CPP_OPTIONS /fsanitize=address)
    endif()
endif()

if(Compiler_MSVC)

    list(APPEND _DEF_C_CPP_OPTIONS
        # /std:c++20
        /Zc:preprocessor # preprocessor conformance mode (https://learn.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview?view=msvc-170)
        /MP  #enable multi processor compilation (which is used only for cl)
    )
endif()



STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(_STRICT_DIAGNOSTIC_OPTIONS)
set(_DIAGNOSTIC_OPTIONS)

if(Compiler_LikeCl)
    set(IgnoreWarnings
        /wd4275 # non dll-interface struct 'XXX' used as base for dll-interface class 'YYY'
        /wd4250 # 'Class_XXX': inherits 'Method_YYY' via dominance
        /wd4251 #  Warning	C4251	'': class '' needs to have dll-interface to be used by clients of struct ''
        /wd4625 #  copy constructor was implicitly defined as deleted
        /wd4435 # 'Class1': Object layout under /vd2 will change due to virtual base 'Interface'
        /wd4626 #  assignment operator was implicitly defined as deleted
        /wd4820 # 'bytes' bytes padding added after construct 'member_name'
        /wd4868 # warning C4866: compiler may not enforce left-to-right evaluation order for call to operator_name
        /wd5026 # move constructor was implicitly defined as deleted
        /wd5027 # move assignment operator was implicitly defined as deleted
        /wd5039 # pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception.
        /wd5045 # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
        
    )



    list(APPEND _STRICT_DIAGNOSTIC_OPTIONS 
        #-Wall -WX 
        -W4
        /permissive-
        ${IgnoreWarnings}
        # /wd4514 /wd4061  /wd4668 /wd4619
        # /wd4365 /wd4127 /wd4302 /wd4242 /wd4244 /wd4265
        # /wd4101 /wd4201 /wd4625 /wd4626 /wd4800 /wd4018
        # /wd4710 /wd4245 /wd4291 /wd4389 /wd4200 /wd4255
        # /wd4711 /wd4062 /wd4355 /wd4640 /wd4305 /wd4324
        # /wd4511 /wd4512 /wd4305 /wd4738 /wd4996 /wd4005
        # /wd4740 /wd4702 /wd4826 /wd4503 /wd4748 /wd4987
        # /wd4574 /wd4554 /wd4471 /wd4350 /wd4370 /wd4371
        # /wd4316 /wd4388 /wd4091 /wd5026 /wd5027 /wd4774
        # /wd4312 /wd4302 /wd4334 /wd5220 /wd5219
        # /wd4464 /wd4463 /wd4589 /wd4595 /wd4435 /wd4319 /wd4311 /wd4267 /wd4477 /wd4777
        # /wd4548 /wd5039 /wd5045 /wd4623 /wd5038 /wd4768
        # /wd4456 # very convenient for casting to the same var within one function (event handler)
        # #/wd444 /wd279 /wd981 /wd383 /wd310 /wd174 /wd111 /wd271 /wd4714
        # /wd5052
        # /wd5204 /wd4577
        # /wd4866 # warning C4866: compiler may not enforce left-to-right evaluation order for call to operator_name
        # /wd5245 # Some functions' overloads aren't used and the compiler warns to remove then
        # /wd5246 # the MSC has the CWG defect #1270 and offers the old initialization behaviour, https://stackoverflow.com/a/70127399
        # /wd5264 # falsely identifies static const variables unused
        # /wd4275 # non dll-interface struct 'XXX' used as base for dll-interface class 'YYY'
        # /wd4250 # 'Class_XXX': inherits 'Method_YYY' via dominance
        
    )

    list(APPEND _DIAGNOSTIC_OPTIONS
        -W3
        ${IgnoreWarnings}
        # /wd4244 /wd4101 /wd4800 /wd4018 /wd4291 /wd4200 /wd4355 /wd4305
        # /wd4996 /wd4005 /wd4740 /wd4748 /wd4324 /wd4503 /wd4574 /wd4554 /wd4316
        # /wd4388 /wd4091 /wd5026 /wd5027 /wd4334
        # /wd4595 /wd4838 /wd4312 /wd4302 /wd4311 /wd4319 /wd4477 /wd5039 /wd5045 /wd4623 /wd5038 /wd4768
        # /wd5204 /wd4577 /wd4267
        # /wd4723 #warning C4723: potential divide by 0
        # /wd4866 # warning C4866: compiler may not enforce left-to-right evaluation order for call to operator_name
        # #warning C4263: 'void B::f(int)' : member function does not override any base class virtual member function
        # #/w14263
        # #warning C4264: 'void A::f(void)' : no override available for virtual member function from base 'A'; function is hidden
        # #/w14264
        # /wd4251 #  Warning	C4251	'': class '' needs to have dll-interface to be used by clients of struct ''
        # /wd4275 # non dll-interface struct 'XXX' used as base for dll-interface class 'YYY'
        # /wd4250 # 'Class_XXX': inherits 'Method_YYY' via dominance
    )
endif()

if(Compiler_ClangCl OR Compiler_Clang)
    #Temporary disabled warnings from clang
    set(CLANG_DISABLED_WARNINGS
        -Wno-deprecated-builtins
        -Wno-reorder-ctor
        -Wno-invalid-offsetof
        -Wno-deprecated-enum-enum-conversion
        -Wno-deprecated-volatile
    )
    list(APPEND
        _STRICT_DIAGNOSTIC_OPTIONS
        ${CLANG_DISABLED_WARNINGS}
    )
    list(APPEND
        _DIAGNOSTIC_OPTIONS
        ${CLANG_DISABLED_WARNINGS}
    )

endif()

if(MSVC)
    set(_CONFIG_OPTIONS_DEBUG /GF /Gy /Gw /Oi /Oy /Od)
    set(_CONFIG_OPTIONS_RELEASE /Ox /GF /Gy /Gw /Oi /Ot /Oy)
    set(_CONFIG_OPTIONS_RELWITHDEBINFO /Od /GS- /GF /Gy /Gw)
endif()

set(_CONFIG_OPTIONS
    $<$<CONFIG:Debug>:${_CONFIG_OPTIONS_DEBUG}>
    $<$<CONFIG:Release>:${_CONFIG_OPTIONS_RELEASE}>
    $<$<CONFIG:RelWithDebInfo>:${_CONFIG_OPTIONS_RELWITHDEBINFO}>
)

if (Compiler_MSVC)
  set(_CONFIG_RTTI_OFF /GR-)
  set(_CONFIG_RTTI_ON /GR)
elseif(Compiler_Clang)
  set(_CONFIG_RTTI_OFF /clang:-fno-rtti)
  set(_CONFIG_RTTI_ON /clang:-f-rtti)
endif()


set(_CONFIG_EXCEPTIONS_OFF /EHsc-)
set(_CONFIG_EXCEPTIONS_ON /EHsc)



# if (NOT ${Config} STREQUAL dbg)
#     nau_set_list(_CONFIG_OPTIONS ${_CONFIG_OPTIONS} ${_VC_CRT_TYPE} )
# else()
#     nau_set_list(_CONFIG_OPTIONS ${_CONFIG_OPTIONS} ${_VC_CRT_TYPE}d )
# endif()

# if (${DriverLinkage} STREQUAL static)
#  	nau_set_list(_CONFIG_OPTIONS ${_CONFIG_OPTIONS} -D_TARGET_STATIC_LIB=1)
# endif()
# if (${StarForce})
#  	nau_set_list(_CONFIG_OPTIONS ${_CONFIG_OPTIONS} -DSTARFORCE_PROTECT)
# endif()
# if (${UseWholeOpt})
#  	nau_set_list(_CONFIG_OPTIONS ${_CONFIG_OPTIONS} /GL)
# endif()

# if (${Analyze})
#  	nau_set_list(_CONFIG_OPTIONS ${_CONFIG_OPTIONS} /analyze)
# endif()

if(MSVC)
    list(APPEND _CONFIG_OPTIONS
        /FS
    )
endif()

if(Compiler_ClangCl)
    list(APPEND _CONFIG_OPTIONS
        /clang:-mavx2
        /clang:-mfma
#        /clang:-fcolor-diagnostics
    )
endif()

if(Compiler_Clang)
    list(APPEND _CONFIG_OPTIONS
        -mavx2
        -mfma
    )
endif()


set(_CPP_OPTIONS
    ${_DEF_C_CPP_OPTIONS}
    ${_DIAGNOSTIC_OPTIONS}
    ${_CONFIG_OPTIONS}
)

set(_CPP_STRICT_OPTIONS
    ${_DEF_C_CPP_OPTIONS}
    ${_STRICT_DIAGNOSTIC_OPTIONS}
    ${_CONFIG_OPTIONS}
)

set(_C_OPTIONS ${_DEF_C_CPP_OPTIONS} ${_DIAGNOSTIC_OPTIONS} ${_CONFIG_OPTIONS})
set(_C_STRICT_OPTIONS ${_DEF_C_CPP_OPTIONS} ${_STRICT_DIAGNOSTIC_OPTIONS} ${_CONFIG_OPTIONS})

if(MSVC)
    list(APPEND _C_OPTIONS /TC)
    list(APPEND _C_STRICT_OPTIONS /TC)
endif()
