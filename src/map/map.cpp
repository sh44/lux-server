#include <lux/util/log.hpp>
#include <lux/common/map.hpp>
//
#include <map/chunk.hpp>
//
#include "map.hpp"

Map::Map(PhysicsEngine &physics_engine, data::Config const &config) :
    generator(physics_engine, config)
{

}

Map::~Map()
{
    for(auto i = chunks.begin() ; i != chunks.end();)
    {
        i = unload_chunk(i);
    }
}

Tile &Map::get_tile(MapPos const &pos)
{
    return get_chunk(to_chk_pos(pos)).tiles[to_chk_idx(pos)];
}

Tile const &Map::get_tile(MapPos const &pos) const
{
    return get_chunk(to_chk_pos(pos)).tiles[to_chk_idx(pos)];
}

Chunk &Map::get_chunk(ChkPos const &pos)
{
    if(chunks.count(pos) == 0) return load_chunk(pos);
    else return chunks.at(pos); //TODO code repetition
}

Chunk const &Map::get_chunk(ChkPos const &pos) const
{
    if(chunks.count(pos) == 0) return load_chunk(pos);
    else return chunks.at(pos);
}

void Map::guarantee_chunk(ChkPos const &pos) const
{
    get_chunk(pos);
}

Chunk &Map::load_chunk(ChkPos const &pos) const
{
    util::log("MAP", util::DEBUG, "loading chunk %zd, %zd, %zd", pos.x, pos.y, pos.z);
    chunks.emplace(pos, Chunk());
    Chunk &chunk = chunks.at(pos);
    generator.generate_chunk(chunk, pos);
    return chunk;
}

Map::ChunkIterator Map::unload_chunk(Map::ChunkIterator const &iter) const
{
    return chunks.erase(iter);
}
