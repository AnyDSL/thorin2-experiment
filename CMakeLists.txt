cmake_minimum_required(VERSION 3.0.0)

# download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)

if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)

if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. 
# This defines the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src ${CMAKE_BINARY_DIR}/googletest-build)

include_directories(.)

add_executable(coc 
    test/bitset.cpp
    test/main.cpp
    test/nominal.cpp
    test/primop.cpp
    test/singleton.cpp
    test/variadic.cpp
    test/variants.cpp
    test/substructural.cpp
    thorin/def.cpp
    thorin/def.h
    thorin/primop.h
    thorin/reduce.cpp
    thorin/reduce.h
    thorin/tables.h
    thorin/type.h
    thorin/qualifier.h
    thorin/world.cpp
    thorin/world.h
    thorin/frontend/token.h
    thorin/frontend/lexer.h
    thorin/frontend/lexer.cpp
    thorin/frontend/parser.h
    thorin/frontend/parser.cpp
    thorin/util/array.h
    thorin/util/bitset.cpp
    thorin/util/bitset.h
    thorin/util/cast.h
    thorin/util/hash.cpp
    thorin/util/hash.h
    thorin/util/location.cpp
    thorin/util/location.h
    thorin/util/stream.cpp
    thorin/util/stream.h
    thorin/util/utility.h
    thorin/util/log.h
    thorin/util/log.cpp
)

set_property(TARGET coc PROPERTY CXX_STANDARD 14)
set_property(TARGET coc PROPERTY CXX_STANDARD_REQUIRED on)

target_link_libraries(coc gtest_main)
add_test(NAME coc_test COMMAND coc)
