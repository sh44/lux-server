#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
//
#include <lux_shared/common.hpp>
//
#include <chunk_mesher.hpp>
#include <marching_cubes.hpp>

static std::thread thread;
static std::atomic<bool> is_running;
static List<DynArr<MesherRequest>> queue;
static std::mutex queue_mutex;
static std::mutex results_mutex;
static std::condition_variable queue_cv;
static VecMap<ChkPos, ChunkMesh> results;

static void generate_mesh(MesherRequest const& request) {
    ChkPos pos = request.pos;
    if(results.count(pos) > 0) return;
    LUX_LOG("building mesh");
    LUX_LOG("    pos: {%zd, %zd, %zd}", pos.x, pos.y, pos.z);

    static VecMap<Vec3<U16>, U16> idx_map;
    idx_map.clear();
    auto& mesh = results[pos];
    mesh.verts.reserve_exactly(CHK_VOL * 5 * 3);
    mesh.idxs.reserve_exactly(CHK_VOL * 5 * 3);

    static DynArr<Vec3F> norms(CHK_VOL * 5 * 3);

    auto get_block_l = [&](Vec3I pos) -> Block const& {
        pos += (Int)1;
        Int idx = pos.x + (pos.y + pos.z * (CHK_SIZE + 3)) * (CHK_SIZE + 3);
        LUX_ASSERT(idx < (Int)arr_len(request.blocks) && idx >= 0);
        return request.blocks[idx];
    };

    for(Uns z = 0; z < CHK_SIZE; ++z) {
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                IdxPos idx_pos(x, y, z);
                Arr<F32, 8> vals;
                vals[0] = s_norm((F32)get_block_l(idx_pos).lvl / 15.f);
                int sig = vals[0] < 0.f;
                bool has_face = false;
                for(Uns j = 1; j < 8; ++j) {
                    IdxPos abs_pos = idx_pos + (IdxPos)marching_cubes_offsets[j];
                    vals[j] = s_norm((F32)get_block_l(abs_pos).lvl / 15.f);
                    if((vals[j] < 0.f) != sig) {
                        has_face = true;
                    }
                }
                if(!has_face) continue;
                Arr<McVert, 5 * 3> cell_trigs;
                Uns cell_trigs_num = marching_cubes(&cell_trigs, vals);
                for(Uns j = 0; j < cell_trigs_num * 3; j += 3) {
                    for(Uns k = 0; k < 3; ++k) {
                        Uns iter = j + k;
                        Vec3F u_pos = cell_trigs[iter].p + (Vec3F)idx_pos;
                        //@NOTE not sure if we should multiply fract by 15,
                        //16 seems more logical, but it produces small cracks
                        Vec3<U16> vert_pos = ((Vec3<U16>)u_pos << (U16)4) |
                            (Vec3<U16>)(fract(u_pos) * 15.f);
                        if(idx_map.count(vert_pos) == 0) {
                            idx_map[vert_pos] = mesh.verts.len;
                            mesh.idxs.emplace(mesh.verts.len);
                            Vec3F off = (Vec3F)idx_pos +
                                marching_cubes_offsets[cell_trigs[iter].idx];
                            //@CONSIDER calculating necessary gradients,
                            //and then interpolating them instead
                            auto interp_lvl = [&](Vec3F p) -> U16 {
                                Vec3I b = floor(p);
                                Vec3I u = ceil(p);
                                Vec3F f = fract(p);
                                Arr<U16, 8> v;
                                v[0] = get_block_l({b.x, b.y, b.z}).lvl;
                                v[1] = get_block_l({u.x, b.y, b.z}).lvl;
                                v[2] = get_block_l({b.x, u.y, b.z}).lvl;
                                v[3] = get_block_l({u.x, u.y, b.z}).lvl;
                                v[4] = get_block_l({b.x, b.y, u.z}).lvl;
                                v[5] = get_block_l({u.x, b.y, u.z}).lvl;
                                v[6] = get_block_l({b.x, u.y, u.z}).lvl;
                                v[7] = get_block_l({u.x, u.y, u.z}).lvl;
                                return trilinear_interp(v, f);
                            };
                            F32 constexpr f = 0.5f;
                            Vec3F norm = Vec3F(
                                interp_lvl(u_pos + Vec3F( f,  0,  0)) -
                                interp_lvl(u_pos + Vec3F(-f,  0,  0)),
                                interp_lvl(u_pos + Vec3F( 0,  f,  0)) -
                                interp_lvl(u_pos + Vec3F( 0, -f,  0)),
                                interp_lvl(u_pos + Vec3F( 0,  0,  f)) -
                                interp_lvl(u_pos + Vec3F( 0,  0, -f)));
                            norm = normalize(norm);
                            Vec3<U8> fp = abs(norm) * (F32)(0x7f);
                            Vec3<U8> cnorm = fp |
                                (Vec3<U8>(step(Vec3F(0.f), norm)) << (U8)7);
                            BlockId id = get_block_l((Vec3F)off).id;
                            mesh.verts.emplace(ChunkMesh::Vert{vert_pos, cnorm,
                                .id = id});
                        } else {
                            mesh.idxs.emplace(idx_map.at(vert_pos));
                        }
                    }
                }
            }
        }
    }
    mesh.verts.shrink_to_fit();
    mesh.idxs.shrink_to_fit();
}

static void thread_main() {
    while(true) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, []{return !queue.empty() || !is_running.load();});
        if(!is_running.load()) {
            break;
        }
        results_mutex.lock();
        auto requests = move(*queue.begin());
        queue.pop_front();
        lock.unlock();
        for(auto const& request : requests) {
            generate_mesh(request);
        }
        queue_cv.notify_one();
        results_mutex.unlock();
    }
}

void mesher_init() {
    is_running.store(true);
    thread = std::thread(&thread_main);
}

void mesher_deinit() {
    is_running.store(false);
    queue_cv.notify_all();
    thread.join();
}

void mesher_enqueue(DynArr<MesherRequest>&& data) {
    queue_mutex.lock();
    queue.emplace_back(move(data));
    queue_mutex.unlock();
    queue_cv.notify_one();
}

void mesher_enqueue_wait(DynArr<MesherRequest>&& data) {
    queue_mutex.lock();
    queue.emplace_front(move(data));
    SizeT size = queue.size();
    queue_mutex.unlock();
    queue_cv.notify_one();
    std::unique_lock<std::mutex> lock(queue_mutex);
    queue_cv.wait(lock, [&]{return queue.size() < size;});
}

VecMap<ChkPos, ChunkMesh>& mesher_lock_results() {
    results_mutex.lock();
    return results;
}

void mesher_unlock_results() {
    results.clear();
    results_mutex.unlock();
}
