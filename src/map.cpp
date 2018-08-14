#include <lux/util/log.hpp>
#include <lux/common/map.hpp>
#include <lux/common/tile.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <map/tile_type.hpp>
#include <map/chunk.hpp>
#include "map.hpp"

Map::Map(PhysicsEngine &physics_engine, data::Config const &config) :
    generator(config),
    db(*config.db),
    physics_engine(physics_engine)
{

}

Map::~Map()
{
    for(auto i = chunks.begin() ; i != chunks.end();)
    {
        i = unload_chunk(i);
    }
}

map::TileType const &Map::get_tile(MapPos const &pos)
{
    return *get_chunk(to_chk_pos(pos)).tiles[to_chk_idx(pos)];
}

map::TileType const &Map::get_tile(MapPos const &pos) const
{
    return *get_chunk(to_chk_pos(pos)).tiles[to_chk_idx(pos)];
}

map::Chunk &Map::get_chunk(ChkPos const &pos)
{
    if(chunks.count(pos) == 0) return load_chunk(pos);
    else return chunks.at(pos); //TODO code repetition
}

map::Chunk const &Map::get_chunk(ChkPos const &pos) const
{
    if(chunks.count(pos) == 0) return load_chunk(pos);
    else return chunks.at(pos);
}

void Map::guarantee_chunk(ChkPos const &pos) const
{
    get_chunk(pos);
}

void Map::try_mesh(ChkPos const &pos) const
{
    constexpr glm::vec3 quads[6][4] =
    {
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}},
        {{1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 1.0}},
        {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}},
        {{0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
        {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
        {{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}}
    };
    constexpr MapPos offsets[6] =
        {{-1,  0,  0}, { 1,  0,  0},
         { 0, -1,  0}, { 0,  1,  0},
         { 0,  0, -1}, { 0,  0,  1}};

    for(SizeT side = 0; side < 6; ++side)
    {
        if(chunks.count(pos + (ChkPos)offsets[side]) == 0) return;
        /* surrounding chunks need to be loaded to mesh */
    }
    map::Chunk &chunk = chunks.at(pos);
    chunk.mesh = new map::Mesh();
    map::Mesh &mesh = *chunk.mesh;
    {
        SizeT worst_case_len = CHK_VOLUME / 2 +
            (CHK_VOLUME % 2 == 0 ? 0 : 1);
        /* this is the size of a checkerboard pattern, the worst case for this
         * algorithm.
         */
        mesh.vertices.reserve(worst_case_len * 6 * 4); //TODO magic numbers
        mesh.indices.reserve(worst_case_len * 6 * 6);  //
    }

    I32 index_offset = 0;
    tile::Id void_id = db.get_tile_id("void");

    auto is_solid = [&] (MapPos const &pos)
    {
        return chunks[to_chk_pos(pos)].tiles[to_chk_idx(pos)]->id != void_id;
    };

    for(SizeT i = 0; i < CHK_VOLUME; ++i)
    {
        MapPos map_pos = to_map_pos(pos, i);
        if(is_solid(map_pos))
        {
            IdxPos idx_pos = to_idx_pos(i);
            for(SizeT side = 0; side < 6; ++side)
            {
                if(!is_solid(map_pos + offsets[side]))
                {
                    for(unsigned j = 0; j < 4; ++j)
                    {
                        mesh.vertices.emplace_back((Vec3<F32>)idx_pos +
                                                   quads[side][j]);
                    }
                    for(auto const &idx : {0, 1, 2, 2, 3, 0})
                    {
                        mesh.indices.emplace_back(idx + index_offset);
                    }
                    index_offset += 4;
                }
            }
        }
    }
    mesh.vertices.shrink_to_fit();
    mesh.indices.shrink_to_fit();

    if(mesh.vertices.size() != 0)
    {
        mesh.bt_trigs = new btTriangleIndexVertexArray(
            mesh.indices.size() / 3,
            mesh.indices.data(),
            sizeof(U32) * 3,
            mesh.vertices.size(),
            (F32 *)mesh.vertices.data(),
            sizeof(Vec3<F32>));
        mesh.bt_mesh = new btBvhTriangleMeshShape(mesh.bt_trigs, false,
                {0.0, 0.0, 0.0}, {CHK_SIZE.x, CHK_SIZE.y, CHK_SIZE.z});
        physics_engine.add_shape(to_map_pos(pos, 0), mesh.bt_mesh);
    }
}

map::Chunk &Map::load_chunk(ChkPos const &pos) const
{
    util::log("MAP", util::DEBUG, "loading chunk %zd, %zd, %zd", pos.x, pos.y, pos.z);
    chunks.emplace(pos, map::Chunk());
    map::Chunk &chunk = chunks.at(pos);
    generator.generate_chunk(chunk, pos);
    constexpr ChkPos offsets[6] =
        {{-1,  0,  0}, { 1,  0,  0},
         { 0, -1,  0}, { 0,  1,  0},
         { 0,  0, -1}, { 0,  0,  1}};

    for(SizeT side = 0; side < 6; ++side)
    {
        try_mesh(pos + offsets[side]);
    }
    return chunk;
}

Map::ChunkIterator Map::unload_chunk(Map::ChunkIterator const &iter) const
{
    return chunks.erase(iter);
}
