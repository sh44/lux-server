local ffi = require("ffi")

local function load_def(path)
    local header = assert(io.open(path, "r"));
    ffi.cdef(header:read("*all"))
    header:close()
end

load_def("api/alias.h")
load_def("api/decl.h")

lux = ffi.load("./libapi.so")

function table_to_hash_map(tab, hash_map)
    for k, v in pairs(tab) do
        lux.hash_map_insert(hash_map, k, v)
    end
end
