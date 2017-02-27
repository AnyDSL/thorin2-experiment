#include "gtest/gtest.h"

#include "thorin/util/log.h"

int main(int argc, char** argv)  {
    ::testing::InitGoogleTest(&argc, argv);
    thorin::Log::set(thorin::Log::Error, &std::cout);
    return RUN_ALL_TESTS();
}
