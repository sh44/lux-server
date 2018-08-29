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

void Map::guarantee_mesh(ChkPos const &pos) const
{
    try_mesh(pos);
}

void Map::try_mesh(ChkPos const &pos) const
{
    constexpr MapPos offsets[6] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    if(chunks.count(pos) == 0) return;
    map::Chunk &chunk = chunks.at(pos);
    if(chunk.mesh != nullptr) return;
    for(SizeT side = 0; side < 3; ++side)
    {
        if(chunks.count(pos + (ChkPos)offsets[side]) == 0) return;
        /* surrounding chunks need to be loaded to mesh */
    }

    chunk.mesh = new map::Mesh();
    map::Mesh &mesh = *chunk.mesh;
    {
        SizeT worst_case_len = CHK_VOLUME / 2;
        /* this is the size of a checkerboard pattern, the worst case for this
         * algorithm.
         */
        mesh.vertices.reserve(worst_case_len * 6 * 4); //TODO magic numbers
        mesh.indices.reserve(worst_case_len * 6 * 6);  //
    }

    I32 index_offset = 0;
    tile::Id void_id = db.get_tile_id("void");

    MapPos base_pos = to_map_pos(pos, 0);
    auto has_face = [&] (MapPos const &a, MapPos const &b) -> bool
    {
        //TODO use current chunk to reduce to_chk_* calls, and chunks access
        return (chunks[to_chk_pos(a)].tiles[to_chk_idx(a)]->id == void_id) !=
               (chunks[to_chk_pos(b)].tiles[to_chk_idx(b)]->id == void_id);
    };
    auto add_quad = [&] (Vec3<U32> const &base, U32 plane, ChkIdx chk_idx,
                         Vec3<U32> const &f_side, Vec3<U32> const &s_side)
    {
        (void)plane;
        (void)chk_idx;
        for(U32 vert_i = 0; vert_i <= 0b11; ++vert_i) {
            Vec2<U32> offset = {vert_i & 0b01, (vert_i & 0b10) >> 1};
            mesh.vertices.emplace_back(
                (glm::vec3)(base + (offset.x * f_side) + (offset.y * s_side)));
        }

        for(auto const &r_idx : {2, 1, 0, 3, 1, 2})
        {
            mesh.indices.emplace_back(r_idx + index_offset);
        }
        index_offset += 4;
    };

    build_chunk_mesh(pos, has_face, add_quad);

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
        physics_engine.add_shape(base_pos, mesh.bt_mesh);
        chunk.has_mesh = true;
    }
    else chunk.has_mesh = false;
}

map::Chunk &Map::load_chunk(ChkPos const &pos) const
{
    util::log("MAP", util::DEBUG, "loading chunk %zd, %zd, %zd", pos.x, pos.y, pos.z);
    chunks.emplace(pos, map::Chunk());
    map::Chunk &chunk = chunks.at(pos);
    generator.generate_chunk(chunk, pos);
    return chunk;
}

Map::ChunkIterator Map::unload_chunk(Map::ChunkIterator const &iter) const
{
    return chunks.erase(iter);
}
