#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
//
#include <lux_shared/noise.hpp>
//
#include <chunk_loader.hpp>

static std::thread thread;

static VecSet<ChkPos> chunk_queue;

static std::mutex queue_mutex;
static std::mutex results_mutex;
static std::mutex block_changes_mutex;
static std::atomic<bool> is_running;
static std::condition_variable queue_cv;

static VecMap<ChkPos, DynArr<BlockChange>> block_changes;
static VecMap<ChkPos, Arr<Block, CHK_VOL>> results;
//@TODO might want to use the excess values somehow
//@TODO derivative function
typedef Arr<F32, (CHK_SIZE + 1) * (CHK_SIZE + 1)> HeightChunk;
static VecMap<Vec2<ChkCoord>, HeightChunk> height_map;

static HeightChunk const& guarantee_height_chunk(Vec2<ChkCoord> const& pos) {
    constexpr Uns octaves    = 16;
    constexpr F32 base_scale = 0.001f;
    constexpr F32 h_exp      = 3.f;
    constexpr F32 max_h      = 512.f;
    if(height_map.count(pos) == 0) {
        auto& h_chunk = height_map[pos];
        Vec2F base_seed = (Vec2F)(pos * (ChkCoord)CHK_SIZE) * base_scale;
        Vec2F seed = base_seed;
        Uns idx = 0;
        for(Uns y = 0; y < CHK_SIZE + 1; ++y) {
            for(Uns x = 0; x < CHK_SIZE + 1; ++x) {
                h_chunk[idx] =
                    pow(u_norm(noise_fbm(seed, octaves)), h_exp) * max_h;
                seed.x += base_scale;
                ++idx;
            }
            seed.x  = base_seed.x;
            seed.y += base_scale;
        }
    }
    auto const& h_chunk = height_map[pos];
    return h_chunk;
}

static void load_chunk(ChkPos const& pos) {
    LUX_LOG("loading chunk");
    LUX_LOG("    pos: {%zd, %zd, %zd}", pos.x, pos.y, pos.z);
    LUX_ASSERT(results.count(pos) == 0);
    auto& chunk = results[pos];
    auto write_suspended_block =
    [&](MapPos const& map_pos, Block const& block) {
        ChkPos chk_pos = to_chk_pos(map_pos);
        ChkIdx idx = to_chk_idx(map_pos);
        if(chk_pos == pos) {
            chunk[idx] = block;
        } else {
            block_changes_mutex.lock();
            block_changes[chk_pos].push({idx, block});
            block_changes_mutex.unlock();
        }
    };
    static const BlockId raw_stone  = db_block_id("raw_stone"_l);
    static const BlockId dark_grass = db_block_id("dark_grass"_l);
    static const BlockId dirt       = db_block_id("dirt"_l);
    static const BlockId snow       = db_block_id("snow"_l);
    Vec2<ChkCoord> h_pos = pos;
    auto const& h_chunk = guarantee_height_chunk(h_pos);
#if 0
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);

        if(map_pos.z <= 0) {
            chunk[i].id  = dark_grass;
            chunk[i].lvl = 0xf;
        } else if(!(pos.x % 2) && !(pos.y % 2) && pos.z == 0) {
            F32 len = distance((Vec3F)map_pos, (Vec3F)to_map_pos(pos, 0) + Vec3F(CHK_SIZE / 2));
            chunk[i].id  = raw_stone;
            chunk[i].lvl = clamp((F32)(CHK_SIZE / 2 - 1) - len, 0.f, 1.f) * 15.f;
        }
    }
#endif
#if 1
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        IdxPos idx_pos = to_idx_pos(i);
        auto get_h = [&](IdxPos p) {
            return h_chunk[p.x + p.y * (CHK_SIZE + 1)];
        };
        F32 h = get_h(idx_pos);
        MapCoord f_h = ceil(h);

        Block block;
        if(map_pos.z > f_h) {
            block.id = void_block;
            block.lvl = 0x0;
        } else {
            F32 diffx = abs(h - get_h(idx_pos + IdxPos(1, 0, 0)));
            F32 diffy = abs(h - get_h(idx_pos + IdxPos(0, 1, 0)));
            F32 diff = (diffx + diffy) / 2.f;
            if(map_pos.z >= f_h - 2 - diff * 10.f) {
                if(diff < 0.7f) {
                    block.id = dark_grass;
                } else if(diff < 1.f) {
                    block.id = dirt;
                } else {
                    block.id = raw_stone;
                }
            } else if(map_pos.z >= f_h - 4) {
                block.id = dirt;
            } else {
                block.id = raw_stone;
            }
            F32 r_h = clamp(h - (F32)map_pos.z, 0.f, 1.f);
            block.lvl = (r_h * 15.f);
        }
        chunk[i] = block;
    }
#endif
    static const BlockId grass = db_block_id("grass"_l);
#if 0
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        if(lux_randf(map_pos, 0) > 0.9999f) {
            Vec3F dir = lux_rand_norm_3(map_pos, 1);
            Vec3F iter = (Vec3F)map_pos;
            for(Uns j = 0; j < 10 * 3; ++j) {
                F32 lvl = max(0.f, 1.f - fract(iter[j % 3])) * 7.f + 8.f;
                write_suspended_block((MapPos)floor(iter), {grass, lvl});
                iter[j % 3] += dir[j % 3];
            }
        }
    }
#endif
#if 1
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapCoord h = round(h_chunk[i & ((CHK_SIZE * CHK_SIZE) - 1)]);
        F32 f_h = ceil(h);
        MapPos map_pos = to_map_pos(pos, i);
        if(map_pos.z == f_h && lux_randf(map_pos) > .995f && chunk[i].id == dark_grass) {
            I32 h = lux_randmm(8, 20, map_pos, 0);
            for(Uns j = 0; j < h; ++j) {
                write_suspended_block(map_pos + MapPos(0, 0, j), {dirt, 0xf});
            }
            for(MapCoord z = -(h / 2 + 2); z <= h / 2; ++z) {
                for(MapCoord y = -5; y <= 5; ++y) {
                    for(MapCoord x = -5; x <= 5; ++x) {
                        F32 diff = (F32)((h / 2) + 2 - z) / 4.f - length(Vec2F(x, y));
                        F32 rand_factor = 0.f;//lux_randf(map_pos, 3, Vec3F(x, y, z));
                        if(diff > 0.f) {
                            diff = min(1.f, diff);
                            write_suspended_block(map_pos + MapPos(x, y, h + z), {grass, diff * 15.f});
                        }
                    }
                }
            }
        }
    }
#endif
#if 1
    MapPos base_pos = to_map_pos(pos, 0);
    if(pos.z < 0) {
        U32 worms_num = lux_randf(pos) > 0.99 ? 1 : 0;
        for(Uns i = 0; i < worms_num; ++i) {
            Vec3F dir;
            U32 len = lux_randmm(10, 500, pos, i, 0);
            dir = lux_rand_norm_3(pos, i, 1);
            Vec3F map_pos = to_map_pos(pos, lux_randm(CHK_VOL, pos, i, 2));
            F32 rad = lux_randfmm(2, 5, pos, i, 3);
            for(Uns j = 0; j < len; ++j) {
                rad = noise_fbm(((F32)(j + i) * 1000.f +
                    length((Vec3F)base_pos)) * 0.05f, 3);
                rad = u_norm(rad) * 9.f + 1.f;
                rad *= 1.f - abs(s_norm((F32)j / (F32)len));
                Vec2<ChkCoord> h_pos = to_chk_pos(map_pos);
                HeightChunk const& h_chk = guarantee_height_chunk(h_pos);
                IdxPos idx_pos = to_idx_pos(map_pos);
                F32 h = h_chk[idx_pos.x + idx_pos.y * (CHK_SIZE + 1)];
                if(map_pos.z > h + rad) {
                    ///we have drilled too far into the surface by now
                    break;
                }
                MapCoord r_rad = ceil(rad);
                for(MapCoord z = -r_rad; z <= r_rad; ++z) {
                    for(MapCoord y = -r_rad; y <= r_rad; ++y) {
                        for(MapCoord x = -r_rad; x <= r_rad; ++x) {
                            if(length(Vec3F(x, y, z)) < rad) {
                                write_suspended_block(floor(map_pos + Vec3F(x, y, z)),
                                    {void_block, 0x00});
                            }
                        }
                    }
                }
                map_pos += dir;
                Vec3F ch = {lux_randf(pos, i, 5, j, 0) - 0.5f,
                            lux_randf(pos, i, 5, j, 1) - 0.5f,
                            lux_randf(pos, i, 5, j, 2) - 0.5f};
                ch *= 0.4f;
                dir += ch;
                dir = normalize(dir);
            }
        }
    }
#endif
    if(block_changes.count(pos) > 0) {
        auto const& changes = block_changes.at(pos);
        for(auto const& change : changes) {
            chunk[change.idx] = change.block;
        }
        block_changes.erase(pos);
    }
}

static void thread_main() {
    while(true) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, []{return !chunk_queue.empty() || !is_running.load();});
        if(!is_running.load()) {
            break;
        }
        results_mutex.lock();
        ChkPos pos = *chunk_queue.begin();
        chunk_queue.erase(chunk_queue.begin());
        lock.unlock();
        load_chunk(pos);
        queue_cv.notify_one();
        results_mutex.unlock();
    }
}

void loader_init() {
    is_running.store(true);
    thread = std::thread(&thread_main);
}

void loader_deinit() {
    is_running.store(false);
    queue_cv.notify_all();
    thread.join();
}

void loader_enqueue(Slice<ChkPos> const& chunks) {
    queue_mutex.lock();
    results_mutex.lock();
    for(auto const& pos : chunks) {
        if(results.count(pos) == 0) {
            chunk_queue.insert(pos);
        }
    }
    results_mutex.unlock();
    queue_mutex.unlock();
    queue_cv.notify_one();
}

void loader_enqueue_wait(Slice<ChkPos> const& chunks) {
    queue_mutex.lock();
    results_mutex.lock();
    for(auto const& pos : chunks) {
        if(results.count(pos) == 0) {
            chunk_queue.insert(pos);
        }
    }
    results_mutex.unlock();
    queue_mutex.unlock();
    queue_cv.notify_one();
    std::unique_lock<std::mutex> lock(queue_mutex);
    queue_cv.wait(lock, []{return chunk_queue.empty();});
}

VecMap<ChkPos, Arr<Block, CHK_VOL>> const& loader_lock_results() {
    results_mutex.lock();
    return results;
}

void loader_unlock_results() {
    results.clear();
    results_mutex.unlock();
}

VecMap<ChkPos, DynArr<BlockChange>>& loader_lock_block_changes() {
    block_changes_mutex.lock();
    return block_changes;
}

void loader_unlock_block_changes() {
    block_changes_mutex.unlock();
}

void loader_write_suspended_block(Block const& block, MapPos const& pos) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    results_mutex.lock();
    if(results.count(chk_pos) > 0) {
        results.at(chk_pos)[chk_idx] = block;
    } else {
        block_changes_mutex.lock();
        block_changes[chk_pos].push({chk_idx, block});
        block_changes_mutex.unlock();
    }
    results_mutex.unlock();
}
