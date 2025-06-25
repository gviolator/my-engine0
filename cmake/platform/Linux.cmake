# Setup for Linux: gcc, clang compilers

set(Platform_Linux ON)
set(Platform_Name linux)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(Compiler_Clang ON)
else()
    message(FATAL_ERROR "Only clang compiler supported: (${CMAKE_CXX_COMPILER_ID})")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(_DEF_C_CPP_DEFINITIONS_DEBUG)

set(_CONFIG_OPTIONS_RELWITHDEBINFO
    NDEBUG=1
)

set(_DEF_C_CPP_DEFINITIONS_RELEASE
    NDEBUG=1
    _SECURE_SCL=0
)

set(_DEF_C_CPP_DEFINITIONS
    $<$<CONFIG:Debug>:${_DEF_C_CPP_DEFINITIONS_DEBUG}>
    $<$<CONFIG:Release>:${_DEF_C_CPP_DEFINITIONS_RELEASE}>
    $<$<CONFIG:RelWithDebInfo>:${_CONFIG_OPTIONS_RELWITHDEBINFO}>
)

set(_DEF_C_CPP_OPTIONS
    -nostdinc++ -nostdlib++
    -isystem /home/ntimofeev/dev/libcpp/debug/include/c++/v1
    #-L /home/ntimofeev/dev/libcpp/debug/lib
    #-Wl,-rpath,/home/ntimofeev/dev/libcpp/debug/lib
)

set(_DIAGNOSTIC_OPTIONS)

set(_STRICT_DIAGNOSTIC_OPTIONS

)


set(_CONFIG_RTTI_OFF -fno-rtti)
set(_CONFIG_RTTI_ON -frtti)


set(_CONFIG_OPTIONS
    -mavx2
    -mfma
)

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

