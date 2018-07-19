#include <lux/util/log.hpp>
#include <lux/common/chunk.hpp>
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

Tile &Map::operator[](map::Pos const &pos)
{
    chunk::Pos   chunk_pos = chunk::to_pos(pos);
    chunk::Index idx       = chunk::to_index(pos);
    return get_chunk(chunk_pos).tiles[idx];
}

Tile const &Map::operator[](map::Pos const &pos) const
{
    chunk::Pos   chunk_pos = chunk::to_pos(pos);
    chunk::Index idx       = chunk::to_index(pos);
    return get_chunk(chunk_pos).tiles[idx];
}

Chunk &Map::load_chunk(chunk::Pos const &pos) const
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

Chunk &Map::get_chunk(chunk::Pos const &pos) const
{
    if(chunks.count(pos) == 0)
    {
        return load_chunk(pos);
    }
    else
    {
        return chunks.at(pos);
    }
}
