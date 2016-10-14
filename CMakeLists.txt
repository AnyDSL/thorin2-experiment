cmake_minimum_required (VERSION 2.6)
project (CoC)
include_directories(.)
add_executable(coc 
    thorin/def.h
    thorin/def.cpp
    thorin/world.h
    thorin/world.cpp
    thorin/util/array.h
    thorin/util/assert.h
    thorin/util/cast.h
    thorin/util/hash.h
    thorin/util/stream.h
)
set_property(TARGET coc PROPERTY CXX_STANDARD 11)
set_property(TARGET coc PROPERTY CXX_STANDARD_REQUIRED on)
