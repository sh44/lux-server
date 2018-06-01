ffi = require("ffi")

local api_header = assert(io.open("api/decl.h", "r"));

lux = ffi.load("./libapi.so")
ffi.cdef(api_header:read("*all"))
api_header:close()
