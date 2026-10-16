// Dedicated MesenCE worker: libretro init/load/run/unload/deinit stay on this
// thread; PS5 Pad polling and all Vulkan calls stay on the frontend thread.
ShelfRuntime::ShelfRuntime(const std::string& directory):root(directory){}
ShelfRuntime::~ShelfRuntime(){
    request_stop.store(true);
    if(worker.joinable())worker.join();
}
bool ShelfRuntime::Start(const std::string& path,fe::Renderer& renderer){
    if(active){error="Game already active";return false;}
    if(worker.joinable()){
        if(!worker_done.load(std::memory_order_acquire)){
            error="Previous HD game still closing; try again shortly";return false;
        }
        worker.join();
    }
    error.clear();worker_error.clear();
    std::map<std::string,std::string> options;
    auto read=[&](const std::string& f){
        std::string text;fe::settings::ReadFile(f,text);
        for(const auto& item:fe::settings::ReadState(text).kv)options[item.first]=item.second;
    };
    read(root+"/gs.ini");
    std::string name=path.substr(path.find_last_of('/')+1);
    auto dot=name.find_last_of('.');
    if(dot!=std::string::npos)name.resize(dot);
    read(root+"/settings/"+name+".ini");
    auto get=[&](const std::string& k,const char* fallback){
        auto it=options.find(k);return it==options.end()?std::string(fallback):it->second;
    };
    const std::string hd_choice=get("NES/HdPacks","false");
    const bool hd_request=hd_choice=="true"||hd_choice=="enabled";
    const bool safe=get("NES/SafeMode","true")!="false";
    const bool hd_enabled=hd_request&&!safe;
    aspect=get("NES/Aspect","4:3");bindings.Load(options);
    sound=get("NES/Sound","true")=="true";
    mesen_diag::write("WORKER_START hd_requested=%d safe=%d hd_enabled=%d game=%s",
        int(hd_request),int(safe),int(hd_enabled),path.c_str());
    menu_atlas=renderer.Atlas();game_texture=nullptr;
    request_stop.store(false);worker_done.store(false);
    worker_ready.store(false);worker_failed.store(false);
    pending_buttons.store(0);actions.store(0);
    {
        std::lock_guard<std::mutex> lock(snapshot_mutex);
        snapshot_pixels.clear();produced=consumed=0;
        shown_width=256;shown_height=240;
    }
    active=true;
    worker=std::thread([this,path,hd_enabled]{RunWorker(path,hd_enabled);});
    return true;
}
void ShelfRuntime::RunWorker(std::string path,bool hd_enabled){
    using Clock=std::chrono::steady_clock;
    const auto begun=Clock::now();
    auto fail=[&](const std::string& msg){
        worker_error=msg;
        mesen_diag::write("WORKER_FAIL %s",msg.c_str());
        worker_failed.store(true,std::memory_order_release);
    };
    {
        // Construct and destroy Session here: no libretro call on frontend.
        Session core(root+"/saves");
        core.SetOption("mesen_hdpacks",hd_enabled?"enabled":"disabled");
        std::vector<uint8_t> rom;std::string read_error;
        mesen_diag::write("GAME_READ_BEGIN");
        if(!ReadRom(path,rom,read_error))fail(read_error);
        else {
            mesen_diag::write("GAME_READ_OK bytes=%zu",rom.size());
            mesen_diag::write("GAME_CORE_LOAD_BEGIN");
            if(!core.Load(path,std::move(rom)))fail(core.error);
            else {
                const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
                    Clock::now()-begun).count();
                mesen_diag::write("PERF_LOAD_DURATION_MS %lld",static_cast<long long>(ms));
                mesen_diag::write("GAME_CORE_LOAD_OK fps=%.3f sample_rate=%.0f",
                    core.fps,core.sample_rate);
                Resampler resampler;resampler.Reset(core.sample_rate);
                std::unique_ptr<ShelfAudio> game_audio;
                if(sound){
                    game_audio=std::make_unique<ShelfAudio>();
                    if(!game_audio->Open()){
                        mesen_diag::write("AUDIO_PORT_FAILED");
                        game_audio.reset();
                    }
                }
                worker_ready.store(true,std::memory_order_release);
                mesen_diag::write("WORKER_READY hd=%d",int(hd_enabled));
                const auto period=std::chrono::duration<double>(
                    1.0/std::clamp(core.fps,40.0,75.0));
                auto next=Clock::now(),last_save=next;
                unsigned runs=0,slow=0;
                while(!request_stop.load(std::memory_order_acquire)){
                    auto now=Clock::now();
                    if(now<next){
                        auto remain=std::chrono::duration_cast<std::chrono::milliseconds>(next-now);
                        std::this_thread::sleep_for(std::min(remain,std::chrono::milliseconds(3)));
                        continue;
                    }
                    if(now>next+std::chrono::milliseconds(80))next=now;
                    next+=std::chrono::duration_cast<Clock::duration>(period);
                    core.buttons[0]=pending_buttons.load(std::memory_order_relaxed);
                    unsigned cmd=actions.exchange(0,std::memory_order_acq_rel);
                    if((cmd&1u)&&!core.SaveState())
                        mesen_diag::write("STATE_SAVE_FAIL %s",core.error.c_str());
                    if((cmd&2u)&&!core.LoadState())
                        mesen_diag::write("STATE_LOAD_FAIL %s",core.error.c_str());
                    const auto frame_begin=Clock::now();
                    core.Run();
                    const auto cost=std::chrono::duration_cast<std::chrono::milliseconds>(
                        Clock::now()-frame_begin).count();
                    ++runs;
                    if(cost>35&&(slow<8||runs%240==0)){
                        ++slow;
                        mesen_diag::write("PERF_FRAME_SLOW frame=%u core_ms=%lld size=%ux%u",
                            runs,static_cast<long long>(cost),core.width,core.height);
                    }
                    if(!core.pixels.empty()){
                        std::lock_guard<std::mutex> lock(snapshot_mutex);
                        snapshot_pixels=core.pixels;
                        snapshot_width=core.width;snapshot_height=core.height;
                        ++produced;
                    }
                    if(game_audio)game_audio->Push(resampler.Process(core.audio));
                    now=Clock::now();
                    if(now-last_save>=std::chrono::seconds(60)){
                        const auto tick=Clock::now();
                        if(!core.SaveBattery())mesen_diag::write("SRAM_FAIL %s",core.error.c_str());
                        const auto write_ms=std::chrono::duration_cast<std::chrono::milliseconds>(
                            Clock::now()-tick).count();
                        if(write_ms>50)mesen_diag::write("PERF_SRAM_SAVE_MS %lld",
                            static_cast<long long>(write_ms));
                        last_save=now;
                    }
                    // Explicitly yield to PS5 system overlays/compositor.
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                game_audio.reset();
                mesen_diag::write("WORKER_STOP frames=%u",runs);
            }
        }
        if(!core.Unload())mesen_diag::write("WORKER_UNLOAD_FAIL %s",core.error.c_str());
    }
    worker_done.store(true,std::memory_order_release);
}
void ShelfRuntime::Stop(fe::Renderer& renderer){
    request_stop.store(true,std::memory_order_release);
    renderer.SetAtlas(menu_atlas);
    if(game_texture)renderer.DestroyTexture(game_texture);
    game_texture=nullptr;active=false;
    // NEVER join here: HD decompression may take >25s.
}
bool ShelfRuntime::Build(fe::Renderer& renderer,fe::FrameDesc& frame,uint32_t raw,double now){
    const auto input=bindings.Update(raw,now);
    if(input.menu){Stop(renderer);return false;}
    pending_buttons.store(input.joypad,std::memory_order_relaxed);
    if(input.save)actions.fetch_or(1u,std::memory_order_release);
    if(input.load)actions.fetch_or(2u,std::memory_order_release);
    if(worker_failed.load(std::memory_order_acquire)){
        error=worker_done.load(std::memory_order_acquire)?worker_error:"MesenCE worker failed";
        Stop(renderer);return false;
    }
    frame=fe::FrameDesc{};frame.fade=1;
    frame.bg.glow_pos[2]=frame.bg.glow_pos[3]=1;
    frame.bg.floor_glow[2]=frame.bg.floor_glow[3]=1;
    frame.bg.misc[0]=float(renderer.width())/renderer.height();frame.bg.misc[1]=0.5f;
    if(!worker_ready.load(std::memory_order_acquire))return true;
    std::vector<uint32_t> pixels;unsigned w=0,h=0;
    {
        std::lock_guard<std::mutex> lock(snapshot_mutex);
        if(produced!=consumed&&!snapshot_pixels.empty()){
            pixels=snapshot_pixels;w=snapshot_width;h=snapshot_height;consumed=produced;
        }
    }
    if(!pixels.empty()){
        auto* next=renderer.CreateTexture(w,h,VK_FORMAT_R8G8B8A8_UNORM,pixels.data());
        if(!next){error="MesenCE frame upload failed";Stop(renderer);return false;}
        renderer.SetAtlas(next);
        if(game_texture)renderer.DestroyTexture(game_texture);
        game_texture=next;shown_width=w;shown_height=h;
    }
    const float sw=float(renderer.width()),sh=float(renderer.height());
    float width=std::min(sw,sh*4.0f/3.0f),height=width*3.0f/4.0f;
    if(aspect=="Integer"){
        const float scale=std::max(1.0f,std::floor(std::min(sw/shown_width,sh/shown_height)));
        width=shown_width*scale;height=shown_height*scale;
    }
    const float x=(sw-width)/2,y=(sh-height)/2;
    auto vertex=[](float a,float b,float u,float v){
        return fe::UiVertex{a,b,u,v,0xffffffff,0,0,0,2};
    };
    frame.ui={vertex(x,y,0,0),vertex(x+width,y,1,0),vertex(x,y+height,0,1),
              vertex(x,y+height,0,1),vertex(x+width,y,1,0),vertex(x+width,y+height,1,1)};
    return true;
}
