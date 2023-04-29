set_languages("c++20")
add_rules("mode.release")
includes("Reflection")
target("gos")
    set_kind("static")
    add_deps("reflection")
    add_files("*.cpp")
    remove_files("main.cpp")
