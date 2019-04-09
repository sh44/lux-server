#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
//
#include <lux_shared/noise.hpp>
//
#include <chunk_loader.hpp>

static std::thread thread;

static VecSet<ChkPos> chunk_queue_set;
static Queue<ChkPos>  chunk_queue;

static std::mutex queue_mutex;
static std::mutex results_mutex;
static std::mutex block_changes_mutex;
static std::atomic<bool> is_running;
static std::condition_variable queue_cv;

static LoaderBlockChanges block_changes;
static LoaderResults results;
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
                    pow(u_norm(noise_fbm(seed, octaves)), h_exp) * max_h +
                    lux_randf(seed) * 0.15f;
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
    Chunk::Data* chunk = lux_alloc<Chunk::Data>(1);
    auto get_block =
    [&](ChkIdx const& idx) -> Block& {
        return chunk->blocks[idx];
    };
    auto write_suspended_block =
    [&](MapPos const& map_pos, Block const& block) {
        ChkPos chk_pos = to_chk_pos(map_pos);
        ChkIdx idx = to_chk_idx(map_pos);
        if(chk_pos == pos) {
            get_block(idx) = block;
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
        } else if(!(pos.x % 2) && !(pos.y % 2) && pos.z == 0) {
            F32 len = distance((Vec3F)map_pos, (Vec3F)to_map_pos(pos, 0) + Vec3F(CHK_SIZE / 2));
            chunk[i].id  = raw_stone;
        }
    }
#endif
#if 1
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        Vec2<MapCoord> h_pos = (Vec2<MapCoord>)map_pos;
        IdxPos idx_pos = to_idx_pos(i);
        auto get_h = [&](IdxPos p) {
            return h_chunk[p.x + p.y * (CHK_SIZE + 1)];
        };
        F32 h = get_h(idx_pos);
        MapCoord f_h = ceil(h);

        Block block;
        if(map_pos.z > f_h) {
            block.id = void_block;
        } else {
            F32 diffx = abs(h - get_h(idx_pos + IdxPos(1, 0, 0)));
            F32 diffy = abs(h - get_h(idx_pos + IdxPos(0, 1, 0)));
            F32 diff = (diffx + diffy) / 2.f;
            if(map_pos.z > f_h - max(diffx, diffy) + 1 && (map_pos.z - 240.f) * 0.005f > noise_fbm((Vec2F)h_pos * 0.09f, 4) - 0.2f) {
                    block.id = snow;
            } else
            if(map_pos.z >= f_h - 2 - diff * 10.f) {
                if(diff < 0.7f && lux_randf(h_pos, 1) < pow(noise_fbm((Vec2F)h_pos * 0.08f, 2), 3.f) + 0.97f) {
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
        }
        get_block(i) = block;
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
                write_suspended_block((MapPos)floor(iter), {grass});
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
        if(map_pos.z == f_h && lux_randf(map_pos) > .995f &&
            get_block(i).id == dark_grass) {
            Uns h = lux_randmm(8, 20, map_pos, 0);
            for(Uns j = 0; j < h; ++j) {
                write_suspended_block(map_pos + MapPos(0, 0, j), {dirt});
            }
            for(MapCoord z = -((Int)h / 2 + 2); z <= (Int)h / 2; ++z) {
                for(MapCoord y = -5; y <= 5; ++y) {
                    for(MapCoord x = -5; x <= 5; ++x) {
                        if((F32)((h / 2) + 2 - z) / 4.f > length(Vec2F(x, y))) {
                            write_suspended_block(map_pos + MapPos(x, y, h + z),
                                                  {grass});
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
                                    {void_block});
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
    block_changes_mutex.lock();
    if(block_changes.count(pos) > 0) {
        auto const& changes = block_changes.at(pos);
        for(auto const& change : changes) {
            get_block(change.idx) = change.block;
        }
        block_changes.erase(pos);
    }
    block_changes_mutex.unlock();
    results_mutex.lock();
    results[pos] = chunk;
    results_mutex.unlock();
}

static void thread_main() {
    while(true) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, []{return !chunk_queue.empty() || !is_running.load();});
        if(!is_running.load()) {
            break;
        }
        ChkPos pos = chunk_queue.front();
        lock.unlock();
        load_chunk(pos);
        lock.lock();
        chunk_queue.pop();
        chunk_queue_set.erase(pos);
        lock.unlock();
        queue_cv.notify_one();
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
        if(results.count(pos) == 0 &&
           chunk_queue_set.count(pos) == 0) {
            chunk_queue.push(pos);
            chunk_queue_set.insert(pos);
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
        if(results.count(pos) == 0 &&
           chunk_queue_set.count(pos) == 0) {
            chunk_queue.push(pos);
            chunk_queue_set.insert(pos);
        }
    }
    results_mutex.unlock();
    queue_mutex.unlock();
    queue_cv.notify_one();
    std::unique_lock<std::mutex> lock(queue_mutex);
    //@XXX this woke up before the queue was empty in the past, as we unlocked
    //the queue_mutex in thread_main before loading the chunk,
    //however right now it seems to work
    queue_cv.wait(lock, []{return chunk_queue.empty();});
}

LoaderResults const& loader_lock_results() {
    results_mutex.lock();
    return results;
}

bool loader_try_lock_results(LoaderResults*& out) {
    if(results_mutex.try_lock()) {
        out = &results;
        return true;
    } else {
        return false;
    }
}

void loader_unlock_results() {
    results.clear();
    results_mutex.unlock();
}

bool loader_try_lock_block_changes(LoaderBlockChanges*& out) {
    if(block_changes_mutex.try_lock()) {
        out = &block_changes;
        return true;
    } else {
        return false;
    }
}

LoaderBlockChanges& loader_lock_block_changes() {
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
        results.at(chk_pos)->blocks[chk_idx] = block;
    } else {
        block_changes_mutex.lock();
        block_changes[chk_pos].push({chk_idx, block});
        block_changes_mutex.unlock();
    }
    results_mutex.unlock();
}
