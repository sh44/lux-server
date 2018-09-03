#include <lux/util/log.hpp>
#include <lux/common/map.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <map/voxel_type.hpp>
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

VoxelId &Map::get_voxel(MapPos const &pos)
{
    return get_chunk(to_chk_pos(pos)).voxels[to_chk_idx(pos)];
}

//TODO we will need to update meshes etc. on change, so this will need a const
//version
Chunk &Map::get_chunk(ChkPos const &pos)
{
    guarantee_chunk(pos);
    return chunks.at(pos);
}

void Map::guarantee_chunk(ChkPos const &pos)
{
    if(chunks.count(pos) == 0) load_chunk(pos);
}

void Map::guarantee_mesh(ChkPos const &pos)
{
    constexpr ChkPos offsets[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    guarantee_chunk(pos);
    Chunk &chunk = chunks.at(pos);
    if(!chunk.is_mesh_generated) {
        for(SizeT a = 0; a < 3; ++a) {
            /* surrounding chunks need to be loaded to mesh, contrary to the
             * client's version, we want to load the physics mesh ASAP,
             * so we don't care about asymmetry */
            guarantee_chunk(pos + offsets[a]);
        }
        build_mesh(chunk, pos);
    }
}

void Map::tick()
{
    lightning_tick();
}

void Map::build_mesh(Chunk &chunk, ChkPos const &pos)
{
    constexpr MapPos offsets[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    Vector<Vec3<F32>> vertices;
    Vector<I32>       indices;

    /* this is the size of a checkerboard pattern, worst case */
    vertices.reserve(CHK_VOLUME * 3 * 4);
    indices.reserve(CHK_VOLUME * 3 * 6);

    I32 index_offset = 0;
    VoxelId void_id = db.get_voxel_id("void");

    MapPos base_pos = to_map_pos(pos, 0);
    auto has_face = [&] (MapPos const &a, MapPos const &b) -> bool
    {
        //TODO use current chunk to reduce to_chk_* calls, and chunks access
        return (chunks[to_chk_pos(a)].voxels[to_chk_idx(a)] == void_id) !=
               (chunks[to_chk_pos(b)].voxels[to_chk_idx(b)] == void_id);
    };

    bool face_map[3][CHK_VOLUME];
    for(U32 i = 0; i < CHK_VOLUME; ++i) {
        IdxPos i_pos = to_idx_pos(i);
        MapPos m_pos = base_pos + (MapPos)i_pos;
        for(U32 a = 0; a < 3; ++a) {
            face_map[a][i] = has_face(m_pos, m_pos + offsets[a]);
        }
    }
    constexpr Vec3<U32> idx_offsets = {1, CHK_SIZE.x, CHK_SIZE.x * CHK_SIZE.y};
    for(U32 a = 0; a < 3; ++a) {
        /* two axes we use to scan the plane for quads (x - 0, y - 1, z - 2)*/
        U32 f_axis = a == 2 ? 0 : a == 0 ? 1 : 2;
        U32 s_axis = a == 1 ? 0 : a == 2 ? 1 : 2;

        /* used to quickly increment chunk index by one on respective axis */
        U32 f_idx_side = idx_offsets[f_axis];
        U32 s_idx_side = idx_offsets[s_axis];

        /* max sizes for axes */
        U32 f_size = CHK_SIZE[f_axis];
        U32 s_size = CHK_SIZE[s_axis];
        for(U32 i = 0; i < CHK_VOLUME; ++i) {
            IdxPos i_pos = to_idx_pos(i);

            if(face_map[a][i]) {
                /* starting coords for scan */
                U32 f_co = i_pos[f_axis];
                U32 s_co = i_pos[s_axis];

                /* get first axis length scan until block without a face is hit
                 * or we exceed axis size */
                U32 f_d_i = 0;
                for(; f_co + f_d_i < f_size; ++f_d_i) {
                    if(!face_map[a][i + f_d_i * f_idx_side]) break;
                }

                /* those will be the resulting quad's sides
                 * second axis is set to illegal value, because it has to be
                 * overriden using min() */
                U32 f_d = f_d_i;
                U32 s_d = s_size;

                /* we find length of the second axis */
                for(U32 f_i = 0; f_i < f_d; ++f_i) {
                    U32 s_d_i = 0;
                    for(; s_co + s_d_i < s_size; ++s_d_i) {
                        if(!face_map[a][i + f_i   * f_idx_side +
                                            s_d_i * s_idx_side]) break;
                    }
                    /* we cannot exceed the smallest side length so far,
                     * otherwise we won't get a rectangle */
                    s_d = std::min(s_d, s_d_i);
                }

                /* now that we have the quad's sides, we set the faces to false,
                 * so that they won't be used to create another quad,
                 * that would result in overlapping quads */
                for(U32 f_i = 0; f_i < f_d; ++f_i) {
                    for(U32 s_i = 0; s_i < s_d; ++s_i) {
                        face_map[a][i + f_i * f_idx_side +
                                        s_i * s_idx_side] = false;
                    }
                }

                Vec3<U32> f_side_vec(0);
                Vec3<U32> s_side_vec(0);
                f_side_vec[f_axis] = f_d;
                s_side_vec[s_axis] = s_d;

                constexpr glm::vec2 quad[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
                for(U32 j = 0; j < 4; ++j) {
                    vertices.emplace_back(
                        (Vec3<F32>)i_pos + (glm::vec3)offsets[a] +
                            quad[j].x * (Vec3<F32>)f_side_vec +
                            quad[j].y * (Vec3<F32>)s_side_vec);
                }

                for(auto const &idx : {0, 1, 2, 2, 3, 0}) {
                    indices.emplace_back(idx + index_offset);
                }
                index_offset += 4;
            }
        }
    }

    vertices.shrink_to_fit();
    indices.shrink_to_fit();

    if(vertices.size() > 0) {
        chunk.mesh      = new Mesh();
        Mesh &mesh = *chunk.mesh;
        vertices.swap(mesh.vertices);
        indices.swap(mesh.indices);
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
    }
    chunk.is_mesh_generated = true;
}

void Map::lightning_tick()
{
    //TODO this list could get out of hand in a long gameplay
    Set<ChkPos> &update_set = lightning_system.update_set;
    for(auto it = update_set.begin(); it != update_set.end();) {
        //TODO add is_loaded
        ChkPos pos = *it;
        if(chunks.count(pos) > 0) {
            lightning_system.update(chunks.at(pos), pos);
            it = update_set.erase(it);
        } else ++it;
    }
}

Chunk &Map::load_chunk(ChkPos const &pos)
{
    util::log("MAP", util::DEBUG, "loading chunk %zd, %zd, %zd", pos.x, pos.y, pos.z);
    chunks.emplace(pos, Chunk());
    Chunk &chunk = chunks.at(pos);
    generator.generate_chunk(chunk, pos);
    if(pos.x % 2 && pos.y % 2) {
    lightning_system.add_node(to_map_pos(pos, IdxPos(8, 8, 2)),
        {0xF, 0x0, 0x0});
    lightning_system.add_node(to_map_pos(pos, IdxPos(8, 10, 2)),
        {0x0, 0x0, 0xF});
    lightning_system.add_node(to_map_pos(pos, IdxPos(12, 8, 2)),
        {0x0, 0xF, 0x0});
    }

    return chunk;
}

Map::ChunkIterator Map::unload_chunk(Map::ChunkIterator const &iter)
{
    if(iter->second.mesh != nullptr) delete iter->second.mesh;
    return chunks.erase(iter);
}
