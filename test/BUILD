load(":tests.bzl", "gen_coc_gtests")

coc_copts = [
    "--std=c++1z",
    "-Wall",
    "-Wextra",
    "-fdiagnostics-color",
]

gen_coc_gtests(
    glob(
        ["*cpp"],
        exclude = ["main.cpp"],
    ),
    coc_copts,
)

cc_binary(
    name = "main",
    srcs = glob(["*.cpp"]),
    copts = coc_copts,
    deps = [
        "//:coc",
        "@gtest//:gtest",
    ],
)
