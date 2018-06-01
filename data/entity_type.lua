require "lux"

entity_type =
{
    new = function(...)
        return ffi.gc(lux.entity_type_ctor(unpack({...})), lux.entity_type_dtor)
    end
}
