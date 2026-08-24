includes("lib/commonlibsse-ng")

set_project("NavigateVRMapFramework")
set_version("0.3.1")
set_license("MIT")
set_languages("c++23")

add_rules("mode.debug", "mode.releasedbg")
add_requires("nlohmann_json v3.12.0")

target("NavigateVRMapFramework")
    set_version("0.3.1")
    add_rules("commonlibsse-ng.plugin", {
        name = "NavigateVR Map Framework",
        author = "Sterlingchapman",
        description = "Modular worldspace-based map selection for NavigateVR"
    })
    add_files("src/**.cpp")
    add_includedirs("src")
    add_packages("nlohmann_json")
    set_pcxxheader("src/pch.h")
