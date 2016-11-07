cmake_minimum_required (VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
project (CoC)
include_directories(.)

add_executable(CoC 
    main.cpp
    matrixtest.cpp
    thorin/def.cpp
    thorin/def.h
    thorin/builder.h
    thorin/world.cpp
    thorin/world.h
    thorin/util/array.h
    thorin/util/assert.h
    thorin/util/cast.h
    thorin/util/hash.h
    thorin/util/stream.cpp
    thorin/util/stream.h
    thorin/visitor.h
    utils/unicodemanager.cpp
    utils/unicodemanager.h
    islplay.cpp)
set_property(TARGET CoC PROPERTY CXX_STANDARD 14)
set_property(TARGET CoC PROPERTY CXX_STANDARD_REQUIRED on)

find_package(ISL REQUIRED)
include_directories(${ISL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ISL_LIBRARY})
target_link_libraries(CoC ${ISL_LIBRARY})
