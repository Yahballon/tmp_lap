/*
 * PS5DreamcastX async cover pipeline for Flycast's ImguiFileTexture.
 * Decode PNG/JPG on ONE worker thread (never from the menu draw loop).
 * Upload complete textures only on the ImGui/render thread (max 2/frame).
 * Keep a small bounded staging cache; the ImGui driver owns GPU cache.
 *
 * This is included from core/ui/gui_util.cpp AFTER loadImage().
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifdef USE_PS5
#include <condition_variable>
#include <deque>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cmath>

namespace dreamcastx_cover_async {
enum class Status { Queued, Decoding, Ready, Failed };
struct Staged {
    Status status = Status::Queued;
    std::vector<u8> rgba;
    int width = 0, height = 0;
    uint64_t serial = 0;
};
class Loader {
public:
    // Returns cached GPU image immediately; otherwise queues a background
    // decode and does not block UI thread on fopen/stbi_load.
    ImTextureID get(const std::string& path, bool nearest, int& budget) {
        if (!imguiDriver || path.empty())
            return ImTextureID();
        ImTextureID resident = imguiDriver->getTexture(path);
        if (resident != ImTextureID())
            return resident;

        std::vector<u8> data;
        int w=0, h=0;
        {
            std::lock_guard<std::mutex> lock(mu);
            auto it = entries.find(path);
            if (it == entries.end()) {
                // Keep the queue bounded even for huge libraries.
                if (entries.size() >= 96)
                    evictOldestUnlocked();
                if (entries.size() < 96) {
                    Staged entry;
                    entry.serial = ++sequence;
                    entries.emplace(path, std::move(entry));
                    queue.push_back(path);
                    ensureWorkerUnlocked();
                    wake.notify_one();
                }
                return ImTextureID();
            }
            it->second.serial = ++sequence;
            if (it->second.status != Status::Ready || budget >= 2)
                return ImTextureID();
            w = it->second.width;
            h = it->second.height;
            data = std::move(it->second.rgba);
            entries.erase(it);
        }
        // Only render thread accesses the graphics driver.
        if (data.empty() || w <= 0 || h <= 0)
            return ImTextureID();
        ++budget;
        try {
            return imguiDriver->updateTextureAndAspectRatio(
                path, data.data(), w, h, nearest);
        } catch (...) {
            // Device/surface changes can invalidate texture uploads;
            // a future menu draw may retry by requesting the path again.
            return ImTextureID();
        }
    }
    void request(const std::string& path, bool urgent) {
        if (path.empty()) return;
        std::lock_guard<std::mutex> lock(mu);
        auto it = entries.find(path);
        if (it != entries.end()) {
            it->second.serial = ++sequence;
            if (urgent && it->second.status == Status::Queued) {
                auto q=std::find(queue.begin(),queue.end(),path);
                if (q!=queue.end()) {
                    queue.erase(q);
                    queue.push_front(path);
                }
            }
            return;
        }
        if (entries.size()>=96) evictOldestUnlocked();
        if (entries.size()>=96) return;
        Staged entry;
        entry.serial=++sequence;
        entries.emplace(path,std::move(entry));
        if (urgent) queue.push_front(path);
        else queue.push_back(path);
        ensureWorkerUnlocked();
        wake.notify_one();
    }
private:
    std::mutex mu;
    std::condition_variable wake;
    std::deque<std::string> queue;
    std::unordered_map<std::string, Staged> entries;
    uint64_t sequence=0;
    bool workerStarted=false;
    void ensureWorkerUnlocked() {
        if (workerStarted) return;
        workerStarted=true;
        // Process lifetime: detached worker and intentionally persistent
        // singleton to avoid blocking console shutdown on slow USB reads.
        std::thread([this]{workerLoop();}).detach();
    }
    void evictOldestUnlocked() {
        auto victim=entries.end();
        for (auto it=entries.begin();it!=entries.end();++it) {
            if (it->second.status==Status::Decoding) continue;
            if (victim==entries.end() ||
                (it->second.status==Status::Ready &&
                 victim->second.status!=Status::Ready) ||
                (it->second.status==victim->second.status &&
                 it->second.serial<victim->second.serial))
                victim=it;
        }
        if (victim!=entries.end()) entries.erase(victim);
    }
    void trimDecodedUnlocked() {
        int ready=0;
        for (auto& kv:entries)
            if (kv.second.status==Status::Ready) ++ready;
        while (ready>12) {
            auto victim=entries.end();
            for (auto it=entries.begin();it!=entries.end();++it)
                if (it->second.status==Status::Ready &&
                    (victim==entries.end() ||
                     it->second.serial<victim->second.serial))
                    victim=it;
            if (victim==entries.end()) break;
            entries.erase(victim);
            --ready;
        }
    }
    void workerLoop() {
        for (;;) {
            std::string path;
            {
                std::unique_lock<std::mutex> lock(mu);
                wake.wait(lock,[this]{return !queue.empty();});
                path=std::move(queue.front());
                queue.pop_front();
                auto it=entries.find(path);
                if (it==entries.end() || it->second.status!=Status::Queued)
                    continue;
                it->second.status=Status::Decoding;
            }
            int w=0,h=0;
            std::vector<u8> decoded;
            u8* raw=loadImage(path,w,h);
            if (raw!=nullptr) {
                // Bound GPU/staging memory and drastically shorten uploads.
                if (w>0 && h>0 && w<=8192 && h<=8192) {
                    const int maxDimension=640;
                    int dw=w,dh=h;
                    if (w>maxDimension || h>maxDimension) {
                        const float factor=(float)maxDimension/std::max(w,h);
                        dw=std::max(1,(int)std::round(w*factor));
                        dh=std::max(1,(int)std::round(h*factor));
                    }
                    decoded.resize((size_t)dw*dh*4);
                    // Nearest-neighbour thumbnail scaling happens OFF the
                    // UI thread; original aspect is retained for imageFill.
                    for (int y=0;y<dh;y++) {
                        const int sy=std::min(h-1,(int)((int64_t)y*h/dh));
                        for (int x=0;x<dw;x++) {
                            const int sx=std::min(w-1,(int)((int64_t)x*w/dw));
                            const size_t from=((size_t)sy*w+sx)*4;
                            const size_t to=((size_t)y*dw+x)*4;
                            decoded[to]=raw[from];
                            decoded[to+1]=raw[from+1];
                            decoded[to+2]=raw[from+2];
                            decoded[to+3]=raw[from+3];
                        }
                    }
                    w=dw;h=dh;
                }
                free(raw);
            }
            {
                std::lock_guard<std::mutex> lock(mu);
                auto it=entries.find(path);
                if(it==entries.end() || it->second.status!=Status::Decoding)
                    continue;
                it->second.width=w;
                it->second.height=h;
                it->second.rgba=std::move(decoded);
                it->second.status=it->second.rgba.empty()?
                                  Status::Failed:Status::Ready;
                trimDecodedUnlocked();
            }
        }
    }
};
inline Loader& loader() {
    // Not destroyed during process exit while detached worker is decoding.
    static Loader* instance=new Loader();
    return *instance;
}
inline bool isCoverPath(const std::string& path) {
    return path.find("/covers/")!=std::string::npos ||
           path.find("/boxart/")!=std::string::npos;
}
inline ImTextureID texture(const std::string& path,bool nearest,int& budget) {
    return loader().get(path,nearest,budget);
}
inline void prioritize(const std::string& path) {
    if (isCoverPath(path)) loader().request(path,true);
}
} // namespace dreamcastx_cover_async
#endif // USE_PS5
