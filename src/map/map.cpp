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
    return this->operator[](chunk::to_pos(pos)).tiles[chunk::to_index(pos)];
}

Tile const &Map::operator[](map::Pos const &pos) const
{
    return this->operator[](chunk::to_pos(pos)).tiles[chunk::to_index(pos)];
}

Chunk &Map::operator[](chunk::Pos const &pos)
{
    if(chunks.count(pos) == 0) return load_chunk(pos);
    else return chunks.at(pos); //TODO code repetition
}

Chunk const &Map::operator[](chunk::Pos const &pos) const
{
    if(chunks.count(pos) == 0) return load_chunk(pos);
    else return chunks.at(pos);
}

void Map::guarantee_chunk(chunk::Pos const &pos) const
{
    this->operator[](pos);
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
