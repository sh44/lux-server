ffi = require "ffi"

require "lux"

require "generator_data"
require "entity_type/human"
require "db"

function load_config(config_ptr)
    config_struct = ffi.cast("ApiConfig *", config_ptr);
    table_to_hash_map(db.tile_types,   config_struct.db.tile_types);
    table_to_hash_map(db.entity_types, config_struct.db.entity_types);
    config_struct.player_type = human
end
