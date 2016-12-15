cmake_minimum_required (VERSION 2.6)
project (CoC)
include_directories(.)
add_executable(CoC 
    main.cpp
    matrixtest.cpp
    nominaltest.cpp
    qualifiertest.cpp
    thorin/def.cpp
    thorin/def.h
    thorin/reduce.cpp
    thorin/reduce.h
    thorin/tables.h
    thorin/world.cpp
    thorin/world.h
    thorin/util/array.h
    thorin/util/bitset.h
    thorin/util/cast.h
    thorin/util/debug.cpp
    thorin/util/debug.h
    thorin/util/hash.h
    thorin/util/stream.cpp
    thorin/util/stream.h
    thorin/util/utility.h
)
set_property(TARGET CoC PROPERTY CXX_STANDARD 14)
set_property(TARGET CoC PROPERTY CXX_STANDARD_REQUIRED on)
