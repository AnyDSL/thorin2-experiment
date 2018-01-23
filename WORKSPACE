workspace(name = "coc")

load(":half.bzl", "half_svn")

gtest_version = "1.8.0"
new_http_archive(
    name = "gtest",
    url = "https://github.com/google/googletest/archive/release-" + gtest_version + ".zip",
    sha256 = "f3ed3b58511efd272eb074a3a6d6fb79d7c2e6a0e374323d1e6bcbcc1ef141bf",
    build_file_content = """
cc_library(
    name = "gtest",
    srcs = [
        "googletest/src/gtest-all.cc",
        "googlemock/src/gmock-all.cc",
    ],
    hdrs = glob([
        "**/*.h",
        "googletest/src/*.cc",
        "googlemock/src/*.cc",
    ]),
    includes = [
        "googlemock",
        "googletest",
        "googletest/include",
        "googlemock/include",
    ],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    # copts = ["-g", "-O0"]
)

cc_library(
    name = "gtest_main",
    srcs = ["googlemock/src/gmock_main.cc"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [":gtest"],
    # copts = ["-g", "-O0"]
)
    """,
    strip_prefix = "googletest-release-" + gtest_version,
)

half_svn(revision = "355", sha256 = "d659163a391183c877d8ea91e8bb683cd9b7286429f2fb31fd24ce4e4cace730")
