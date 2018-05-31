#include <world/map/chunk/index.hpp>
#include <world/map/chunk.hpp>
//
#include "map.hpp"

namespace world::map
{

Tile &Map::operator[](Point pos)
{
    chunk::Point chunk_pos   = Chunk::point_map_to_chunk(pos);
    chunk::Index chunk_index = Chunk::point_map_to_index(pos);
    return get_chunk(chunk_pos).tiles[chunk_index];
}

Chunk &Map::load_chunk(chunk::Point pos)
{
    chunks.emplace(pos, (Tile *)::operator new(sizeof(Tile) * Chunk::TILE_SIZE));
    generator.generate_chunk(chunks.at(pos), pos);
}

void Map::unload_chunk(chunk::Point pos)
{
    auto chunk = chunks.at(pos);
    for(std::size_t i = 0; i < Chunk::TILE_SIZE; ++i)
    {
        (*(chunk.tiles + i)).~Tile();
    }
    ::operator delete[](chunk.tiles);
    chunks.erase(pos);
}

Chunk &Map::get_chunk(chunk::Point pos)
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
