include(CheckCXXCompilerFlag)

set(Compiler_MSVC OFF)
set(Compiler_Cl OFF)
set(Compiler_Clang OFF)
set(Compiler_ClangCl OFF)

set(Platform_Windows OFF)
set(Platform_Win32 OFF)
set(Platform_Win64 OFF)

set(Platform_Linux OFF)
set(Platform_Name "UNKNOWN")

if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(Host_Arch "x86")
else()
    set(Host_Arch "x64")
endif()

#message(FATAL_ERROR "THE_OS: (${OS})")

# if (DEFINED OS)
#     if (OS STREQUAL NT)
#         if (${Host_Arch} STREQUAL x64)
#             nau_conditional_set(Platform win64)
#         else()
#             nau_conditional_set(Platform win32)
#         endif()

#     elseif (OS STREQUAL MACOSX)
#         nau_conditional_set(Platform macosx)

#     elseif (OS STREQUAL LINUX)
#         nau_conditional_set(Platform linux64)
#     endif()
# endif()

#TODO: check ${Platform} instead of cmake values (actual for cross-compilation)
if (WIN32)
  include(platform/Microsoft)
  message(STATUS "Configure for Microsoft/(${CMAKE_SYSTEM_NAME}), msvc:(${Compiler_MSVC}), clang-cl:(${Compiler_ClangCl}), cl like:(${Compiler_Cl})")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  include(platform/Linux)
else()
  message(FATAL_ERROR "Unsupported platform (${CMAKE_SYSTEM_NAME})")
endif()
