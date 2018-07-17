#include <lux/alias/scalar.hpp>
#include <lux/util/log.hpp>
#include <lux/common/chunk.hpp>
//
#include <map/chunk.hpp>
//
#include "map.hpp"

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

Tile &Map::operator[](MapPos const &pos)
{
    ChunkPos chunk_pos     = to_chunk_pos(pos);
    ChunkIndex chunk_index = to_chunk_index(pos);
    return get_chunk(chunk_pos).tiles[chunk_index];
}

Tile const &Map::operator[](MapPos const &pos) const
{
    ChunkPos chunk_pos     = to_chunk_pos(pos);
    ChunkIndex chunk_index = to_chunk_index(pos);
    return get_chunk(chunk_pos).tiles[chunk_index];
}

Chunk &Map::load_chunk(ChunkPos const &pos) const
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

Chunk &Map::get_chunk(ChunkPos const &pos) const
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
