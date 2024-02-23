local lm = require "luamake"

lm:lua_source "scene" {
    includes = {
        lm.AntDir .. "/clibs/ecs",
        lm.AntDir .. "/3rd/math3d",
        lm.AntDir .. "/3rd/luaecs",
        lm.AntDir .. "/3rd/glm",
    },
    sources = {
        "scene.cpp"
    },
    deps = "foundation",
    objdeps = "compile_ecs",
}
