cmake_minimum_required (VERSION 2.6)
project (CoC)
include_directories(.)
add_executable(CoC 
    main.cpp
    matrixtest.cpp
    thorin/def.cpp
    thorin/def.h
    thorin/world.cpp
    thorin/world.h
    thorin/util/array.h
    thorin/util/assert.h
    thorin/util/cast.h
    thorin/util/hash.h
    thorin/util/stream.cpp
    thorin/util/stream.h
    utils/unicodemanager.cpp
    utils/unicodemanager.h
)
set_property(TARGET CoC PROPERTY CXX_STANDARD 11)
set_property(TARGET CoC PROPERTY CXX_STANDARD_REQUIRED on)
