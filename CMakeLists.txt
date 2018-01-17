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
    test/arity.cpp
    test/bitset.cpp
    test/lambda.cpp
    test/lexer.cpp
    test/main.cpp
    test/nominal.cpp
    test/pack.cpp
    test/parser.cpp
    test/primop.cpp
    test/sigma.cpp
    test/singleton.cpp
    test/substructural.cpp
    test/variadic.cpp
    test/variants.cpp
    thorin/def.cpp
    thorin/def.h
    thorin/qualifier.h
    thorin/reduce.cpp
    thorin/reduce.h
    thorin/world.cpp
    thorin/world.h
    thorin/core/normalize.cpp
    thorin/core/normalize.h
    thorin/core/tables.h
    thorin/core/world.cpp
    thorin/core/world.h
    thorin/frontend/lexer.cpp
    thorin/frontend/lexer.h
    thorin/frontend/parser.cpp
    thorin/frontend/parser.h
    thorin/frontend/token.h
    thorin/frontend/token.cpp
    thorin/util/array.h
    thorin/util/bitset.cpp
    thorin/util/bitset.h
    thorin/util/cast.h
    thorin/util/hash.h
    thorin/util/hash.cpp
    thorin/util/location.cpp
    thorin/util/location.h
    thorin/util/log.cpp
    thorin/util/log.h
    thorin/util/stream.cpp
    thorin/util/stream.h
    thorin/util/utility.h
)

set_target_properties(coc PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(coc gtest_main)
add_test(NAME coc_test COMMAND coc)
