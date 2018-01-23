def gen_coc_gtests(cpp_files, copts):
    for test_file in cpp_files:
        native.cc_test(
            name = test_file.replace(".cpp", ""),
            size = "small",
            srcs = [test_file, "main.cpp"],
            deps = ["//:coc", "@gtest//:gtest"],
            copts = copts
        )
