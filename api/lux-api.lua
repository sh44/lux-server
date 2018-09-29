local ffi = require("ffi")

local function load_header(path)
    local header = assert(io.open(path, "r"))
    ffi.cdef(header:read("*all"))
    header:close()
end

load_header("api/decl.h")

local lux_c = ffi.load("api/liblux-api.so")

lux = {}
lux.make_admin = function(name)
    assert(type(name) == "string")
    lux_c.make_admin(name)
end
lux.place_light = function(x, y, z, r, g, b)
    assert(type(x) == "number")
    assert(type(y) == "number")
    assert(type(z) == "number")
    assert(type(r) == "number")
    assert(type(g) == "number")
    assert(type(b) == "number")
    assert(r >= 0 and r < 256);
    assert(g >= 0 and g < 256);
    assert(b >= 0 and b < 256);
    lux_c.place_light(x, y, z, r, g, b);
end

lux.mt = {}
lux.mt.__index = function(tab, k)
    if type(rawget(tab, k)) ~= "nil" then
        return rawget(tab, k)
    else
        return lux_c[k]
    end
end

setmetatable(lux, lux.mt)
