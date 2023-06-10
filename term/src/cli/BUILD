cc_library(
    name = "error",
    srcs = [
        "error.cpp",
    ],
    hdrs = [
        "error.h",
    ],
)

cc_library(
    name = "token",
    hdrs = [
        "token.h",
    ],
    visibility = [
        "//tests/cli:__pkg__",
    ],
)

cc_library(
    name = "parser",
    hdrs = [
        "parser.h",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":error",
        ":token",
    ],
)
