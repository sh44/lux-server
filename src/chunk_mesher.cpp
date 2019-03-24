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

    auto& mesh = results[pos];
    mesh.faces.reserve_exactly(CHK_VOL * 3);

    auto get_block_l = [&](Vec3U pos) -> Block const& {
        Uns idx = pos.x + (pos.y + pos.z * (CHK_SIZE + 1)) * (CHK_SIZE + 1);
        LUX_ASSERT(idx < arr_len(request.blocks));
        return request.blocks[idx];
    };

    Arr<IdxPos, 3> a_off = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    //@TODO single for loop and chk_idx_axis_off
    for(Uns z = 0; z < CHK_SIZE; ++z) {
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                for(Uns a = 0; a < 3; ++a) {
                    IdxPos idx_pos(x, y, z);
                    auto const& b0 = get_block_l(idx_pos);
                    auto const& b1 = get_block_l(idx_pos + a_off[a]);
                    if((b0.id == void_block) != (b1.id == void_block)) {
                        U8 orient = b0.id != void_block;
                        BlockId id = orient ? b0.id : b1.id;
                        mesh.faces.push({to_chk_idx(idx_pos), id,
                            (a << 1 | orient)});
                    }
                }
            }
        }
    }
    mesh.faces.shrink_to_fit();
}

static void thread_main() {
    while(true) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, []{return !queue.empty() || !is_running.load();});
        if(!is_running.load()) {
            break;
        }
        //@TODO narrow down this mutex, so that physics mesher on main thread
        //waits less
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

bool mesher_try_lock_results(MesherResults*& out) {
    if(results_mutex.try_lock()) {
        out = &results;
        return true;
    } else {
        return false;
    }
}

VecMap<ChkPos, ChunkMesh>& mesher_lock_results() {
    results_mutex.lock();
    return results;
}

void mesher_unlock_results() {
    results.clear();
    results_mutex.unlock();
}
