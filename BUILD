# Bazel build file for coc

src_dirs = [
    "thorin/",
    "thorin/**/",
]

exclude_patterns = [
    "thorin/util/ycomp.cpp",
]

coc_srcs = glob(
    [d + "*.cpp" for d in src_dirs],
    exclude = exclude_patterns,
)

coc_hdrs = glob(
    [d + "*.h" for d in src_dirs],
    exclude = exclude_patterns,
)

coc_copts = [
    "--std=c++1z",
    "-Wall",
    "-Wextra",
    "-fdiagnostics-color",
]

coc_deps = [
    "@half//:half",
]

cc_library(
    name = "coc",
    srcs = coc_srcs,
    hdrs = coc_hdrs,
    copts = coc_copts,
    visibility = ["//visibility:public"],
    deps = coc_deps,
)
