add_subdirectory(src)

if (MY_ENGINE_TESTS)
    add_subdirectory(tests/test_lua_toolkit)
endif()
