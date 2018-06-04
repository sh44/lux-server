require "ffi"

entity_type =
{
    new = function(...)
        return ffi.new("ApiEntityType", unpack({...}))
    end
}
