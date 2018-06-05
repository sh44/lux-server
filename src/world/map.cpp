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
    for(auto &i : chunks)
    {
        unload_chunk(i.first);
    }
}

Tile &Map::operator[](MapPoint pos)
{
    ChunkPoint chunk_pos   = Chunk::point_map_to_chunk(pos);
    ChunkIndex chunk_index = Chunk::point_map_to_index(pos);
    util::log("MAP", util::TRACE, "idx %zu", chunk_index);
    return get_chunk(chunk_pos).tiles[chunk_index];
}

Tile const &Map::operator[](MapPoint pos) const
{
    ChunkPoint chunk_pos   = Chunk::point_map_to_chunk(pos);
    ChunkIndex chunk_index = Chunk::point_map_to_index(pos);
    return get_chunk(chunk_pos).tiles[chunk_index];
}

Chunk &Map::load_chunk(ChunkPoint pos) const
{
    util::log("MAP", util::DEBUG, "loading chunk %zd, %zd, %zd", pos.x, pos.y, pos.z);
    chunks.emplace(pos, (Tile *)::operator new(sizeof(Tile) * Chunk::TILE_SIZE));
    Chunk &chunk = chunks.at(pos);
    generator.generate_chunk(chunk, pos);
    return chunk;
}

void Map::unload_chunk(ChunkPoint pos) const
{
    auto &chunk = chunks.at(pos);
    for(SizeT i = 0; i < Chunk::TILE_SIZE; ++i)
    {
        (*(chunk.tiles + i)).~Tile();
    }
    ::operator delete[](chunk.tiles);
    chunks.erase(pos);
}

Chunk &Map::get_chunk(ChunkPoint pos) const
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
