cmake_minimum_required(VERSION 3.10)
project(thorin VERSION 2.0 DESCRIPTION "the higher-order intermediate representation")

option(BUILD_SHARED_LIBS "Build libthorin as shared library (so/dll)" ON)
option(BUILD_TESTS "Build gtest-based test program" ON)

# download and unpack half at configure time
configure_file(CMakeLists.txt.half.in half-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/half-download)

if(result)
    message(FATAL_ERROR "CMake step for half failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/half-download)

if(result)
    message(FATAL_ERROR "Build step for half failed: ${result}")
endif()

add_library(thorin
    thorin/def.cpp
    thorin/def.h
    thorin/normalize.cpp
    thorin/normalize.h
    thorin/qualifier.h
    thorin/tables.h
    thorin/world.cpp
    thorin/world.h
    thorin/me/fold.h
    thorin/me/normalize.cpp
    thorin/me/normalize.h
    thorin/me/tables.h
    thorin/me/world.cpp
    thorin/me/world.h
    thorin/analyses/cfg.cpp
    thorin/analyses/cfg.h
    thorin/analyses/domfrontier.cpp
    thorin/analyses/domfrontier.h
    thorin/analyses/domtree.cpp
    thorin/analyses/domtree.h
    thorin/analyses/looptree.cpp
    thorin/analyses/looptree.h
    thorin/analyses/schedule.cpp
    thorin/analyses/schedule.h
    thorin/analyses/scope.cpp
    thorin/analyses/scope.h
    thorin/analyses/verify.cpp
    thorin/analyses/verify.h
    thorin/fe/lexer.cpp
    thorin/fe/lexer.h
    thorin/fe/parser.cpp
    thorin/fe/parser.h
    thorin/fe/token.h
    thorin/fe/token.cpp
    thorin/transform/import.cpp
    thorin/transform/import.h
    thorin/transform/mangle.cpp
    thorin/transform/mangle.h
    thorin/transform/reduce.cpp
    thorin/transform/reduce.h
    thorin/util/array.h
    thorin/util/bitset.cpp
    thorin/util/bitset.h
    thorin/util/cast.h
    thorin/util/hash.cpp
    thorin/util/hash.h
    thorin/util/indexmap.h
    thorin/util/indexset.h
    thorin/util/iterator.h
    thorin/util/location.cpp
    thorin/util/location.h
    thorin/util/log.cpp
    thorin/util/log.h
    thorin/util/stream.cpp
    thorin/util/stream.h
    thorin/util/symbol.cpp
    thorin/util/symbol.h
    thorin/util/types.h
    thorin/util/utility.h
)

set_target_properties(thorin PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
set_target_properties(thorin PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(thorin PROPERTIES SOVERSION 2)
target_include_directories(thorin PRIVATE . ${CMAKE_BINARY_DIR}/half-src/include)

if(BUILD_TESTS)
    include(CTest)
    include(GoogleTest)

    # download and unpack gtest at configure time
    configure_file(CMakeLists.txt.gtest.in googletest-download/CMakeLists.txt)

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

    set(THORIN_GTEST_SOURCES
        test/arity.cpp
        test/bitset.cpp
        test/cn.cpp
        test/lambda.cpp
        test/main.cpp
        test/nominal.cpp
        test/normalize.cpp
        test/pack.cpp
        test/sigma.cpp
        test/singleton.cpp
        test/substructural.cpp
        test/subtypes.cpp
        test/variadic.cpp
        test/variants.cpp
        test/fe/lexer.cpp
        test/fe/parser.cpp
        test/me/primop.cpp
    )

    add_executable(thorin-gtest ${THORIN_GTEST_SOURCES})
    set_target_properties(thorin-gtest PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    target_include_directories(thorin-gtest PRIVATE . ${CMAKE_BINARY_DIR}/half-src/include)
    target_link_libraries(thorin-gtest thorin gtest_main)
    gtest_discover_tests(thorin-gtest)
endif()
