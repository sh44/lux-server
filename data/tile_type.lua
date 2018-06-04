require "lux"

tile_type =
{
    new = function(...)
        return ffi.new("ApiTileType", unpack({...}))
    end
}
