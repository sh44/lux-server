#include <alias/int.hpp>
#include <util/log.hpp>
#include <world/map/chunk/common.hpp>
#include <world/map/chunk.hpp>
//
#include "map.hpp"

namespace world
{

Map::Map(data::Config const &config) :
    generator(config)
{

}

Map::~Map()
{
    for(auto i = chunks.begin() ; i != chunks.end();)
    {
        i = unload_chunk(i);
    }
}

Tile &Map::operator[](MapPoint const &pos)
{
    ChunkPoint chunk_pos   = Chunk::point_map_to_chunk(pos);
    ChunkIndex chunk_index = Chunk::point_map_to_index(pos);
    util::log("MAP", util::TRACE, "idx %zu", chunk_index);
    return get_chunk(chunk_pos).tiles[chunk_index];
}

Tile const &Map::operator[](MapPoint const &pos) const
{
    ChunkPoint chunk_pos   = Chunk::point_map_to_chunk(pos);
    ChunkIndex chunk_index = Chunk::point_map_to_index(pos);
    return get_chunk(chunk_pos).tiles[chunk_index];
}

Chunk &Map::load_chunk(ChunkPoint const &pos) const
{
    util::log("MAP", util::DEBUG, "loading chunk %zd, %zd, %zd", pos.x, pos.y, pos.z);
    chunks.emplace(pos, (Tile*)::operator new(sizeof(Tile) * Chunk::TILE_SIZE));
    Chunk &chunk = chunks.at(pos);
    generator.generate_chunk(chunk, pos);
    return chunk;
}

Map::ChunkIterator Map::unload_chunk(Map::ChunkIterator const &iter) const
{
    Chunk &chunk = iter->second;
    for(SizeT i = 0; i < Chunk::TILE_SIZE; ++i)
    {
        (*(chunk.tiles + i)).~Tile();
    }
    ::operator delete(chunk.tiles);
    return chunks.erase(iter);
}

Chunk &Map::get_chunk(ChunkPoint const &pos) const
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

}
