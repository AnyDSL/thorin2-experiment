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

add_executable(coc-gtest
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
    test/ssa.cpp
    test/substructural.cpp
    test/variadic.cpp
    test/variants.cpp
    test/fe/lexer.cpp
    test/fe/parser.cpp
    test/me/primop.cpp
    thorin/def.cpp
    thorin/def.h
    thorin/normalize.cpp
    thorin/normalize.h
    thorin/qualifier.h
    thorin/tables.h
    thorin/world.cpp
    thorin/world.h
    thorin/core/fold.h
    thorin/core/normalize.cpp
    thorin/core/normalize.h
    thorin/core/tables.h
    thorin/core/world.cpp
    thorin/core/world.h
    thorin/core/transform/ssa_builder.cpp
    thorin/core/transform/ssa_builder.h
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

set_target_properties(coc-gtest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(coc-gtest gtest_main)
add_test(NAME coc_test COMMAND coc)
