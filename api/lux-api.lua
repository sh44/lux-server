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

lux.mt = {}
lux.mt.__index = function(tab, k)
    if type(rawget(tab, k)) ~= "nil" then
        return rawget(tab, k)
    else
        return lux_c[k]
    end
end

setmetatable(lux, lux.mt)
