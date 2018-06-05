#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#undef GTEST_TEST_NO_THROW_

// From: https://github.com/google/googletest/pull/911
#define GTEST_TEST_NO_THROW_(statement, fail)                           \
    std::string GTEST_CONCAT_TOKEN_(gtest_exmsg_testnothrow_,__LINE__); \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                       \
    if (::testing::internal::AlwaysTrue()) {                            \
        try {                                                           \
            GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement);  \
        }                                                               \
        catch (const std::exception& e) {                               \
            GTEST_CONCAT_TOKEN_(gtest_exmsg_testnothrow_,__LINE__) = e.what(); \
            goto GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__); \
        }                                                               \
        catch (...) {                                                   \
            GTEST_CONCAT_TOKEN_(gtest_exmsg_testnothrow_,__LINE__) = "no std::exception"; \
            goto GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__); \
        }                                                               \
    } else                                                              \
        GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__):        \
            fail(("Expected: " #statement " doesn't throw an exception.\n" \
                  "  Actual: it throws: " + GTEST_CONCAT_TOKEN_(gtest_exmsg_testnothrow_,__LINE__)).c_str())

#endif
