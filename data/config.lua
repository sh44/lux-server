ffi = require "ffi"

require "lux"

require "default_generator"
require "entity_type/human"

config =
{
    generator   = default_generator,
    player_type = human,
}

function load_config()
    config_struct = ffi.new("Config", config_ptr)
    config_struct.player_type = config.player_type
    return config_struct
end
