def _half_svn_impl(ctx):
    ctx.download("https://sourceforge.net/p/half/code/" + ctx.attr.revision + "/tree/trunk/include/half.hpp?format=raw",
                 "half.hpp", ctx.attr.sha256)
    ctx.file("BUILD", """
cc_library(
    name = "half",
    srcs = [],
    hdrs = ["half.hpp"],
    includes = ["."],
    linkstatic=1,
    visibility = ["//visibility:public"])""", False)

_half_svn = repository_rule(
    attrs = {
        "revision": attr.string(mandatory = True),
        "sha256": attr.string(mandatory = True),
    },
    local = False,
    implementation = _half_svn_impl,
)

def half_svn(revision, sha256):
    _half_svn(name = "half", revision = revision, sha256 = sha256)
