#!/usr/bin/env python3
"""Build isolated PS5MesenX frontend sources from pinned PS5SNESX.

Only source preparation: does NOT produce a signed PS5 executable.
"""
from pathlib import Path
import json
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / "upstream/PS5SNESX"
OUTPUT = ROOT / "build/port-src"
TITLE_ID = "PPSA99896"
TITLE = "PS5MesenX"

def replace_required(path, old, new):
    contents = path.read_text()
    if old not in contents:
        raise RuntimeError(f"Missing required upstream patch point: {path.name}: {old[:80]!r}")
    path.write_text(contents.replace(old, new))

def generate():
    if OUTPUT.exists():
        shutil.rmtree(OUTPUT)
    shutil.copytree(ORIGINAL, OUTPUT, symlinks=True)
    # The pinned PS5SNESX revision predates build-mesa-util.sh. Vendor the
    # independently licensed helper from upstream and fail early if absent.
    mesa_helper = ROOT / "vendor/build-mesa-util.sh"
    if not mesa_helper.is_file():
        raise RuntimeError("Missing PS5 Mesa utility build helper")
    helper_target = OUTPUT / "ps5/tools/build-mesa-util.sh"
    helper_target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(mesa_helper, helper_target)
    helper_target.chmod(0o755)
    source = OUTPUT / "snes/src"
    for name in ("core_session.h", "core_session.cpp"):
        shutil.copy2(ROOT / "src" / ("libretro_" + name), source / name)

    # Correct game detection: SNES-only validation must never reject iNES.
    rom = source / "games.cpp"
    replace_required(rom,
        'return ext==".sfc"||ext==".smc"||ext==".swc"||ext==".fig";',
        'return ext==".nes";')
    replace_required(rom, "ZIP contains no SNES ROM", "ZIP contains no NES ROM")
    before = rom.read_text()
    first = before.index('    // (AI-assisted) Validate normal SNES cartridge headers')
    last = before.index('    error.clear();return true;', first)
    after = '''    // Accept iNES/NES2.0 header only; ignore unrelated SNES cart heuristics.
    if (rom.size() < 16 || rom.size() > (64u << 20) ||
        std::memcmp(rom.data(), "NES\\x1A", 4) != 0) {
        error = "Not a valid iNES/NES2 ROM"; rom.clear(); return false;
    }
'''
    rom.write_text(before[:first] + after + before[last:])
    # Reuse the battle-tested PS5NESX local file scanning and loading paths.
    # They support DT_REG, DT_UNKNOWN, fd/fstat and readable NES headers when
    # Sony PS5 mounts reject path metadata syscalls.
    reference = ROOT / "vendor/ps5nesx_games_reference.cpp"
    expected = reference.read_text()
    first = expected.index('    DIR* dir=opendir(path.c_str());')
    last = expected.index(chr(10) + '}' + chr(10) + 'std::vector<Game> Scan(', first)
    working_scan = expected[first:last]
    first = expected.index('    } else {' + chr(10) + '        const int fd=open(path.c_str(),O_RDONLY);', expected.index('bool ReadRom('))
    last = expected.index(chr(10) + '    if(path.size()>4&&Lower(', first)
    working_load = expected[first:last]
    original = rom.read_text()
    start = original.index('    DIR* dir=opendir(path.c_str());')
    end = original.index(chr(10) + '}' + chr(10) + 'std::vector<Game> Scan(', start)
    original = original[:start] + working_scan + original[end:]
    start = original.index('    } else {' + chr(10) + '        const int fd=open(path.c_str(),O_RDONLY);', original.index('bool ReadRom('))
    end = original.index(chr(10) + '    if(path.size()>4&&Lower(', start)
    original = original[:start] + working_load + original[end:]
    rom.write_text(original)
    # PS5NESX reports per-folder discovery counts for the actual native scan.
    assert "[library] local %s: matched=" in rom.read_text()


    shelf = source / "shelf_games.cpp"
    replace_required(shelf, 's==".sfc"||s==".smc"||s==".swc"||s==".fig"||s==".zip"',
                     's==".nes"||s==".zip"')
    replace_required(shelf, 'SNES-%08X', 'MESEN-%08X')
    replace_required(shelf, 'SNES ROM (FCEUmm)', 'NES ROM (MesenCE)')
    replace_required(shelf, 'game.badges={"SNES"};', 'game.badges={"NES"};')
    # Show exactly which directories the PS5 scan is asked to traverse.
    replace_required(shelf, 'auto dirs=directories;std::string root;',
        'auto dirs=directories;std::string root;for(const auto& d:dirs)std::printf("[library] scan source: %s\\n",d.c_str());')


    # Never let an old 0-game cache mask the library. Preserve live scan
    # progress instead of reporting 0 / 0 after a cached-library rescan.
    shelf_body = shelf.read_text()
    cache_start = shelf_body.index('    const std::string cache=root.empty()?')
    cache_end = shelf_body.index('    std::vector<GameInfo> games;for(const auto& game:listed)',cache_start)
    shelf_body = (shelf_body[:cache_start] +
        '''    // PS5MesenX: always rescan local NES folders on startup.
    mesen_diag::write("SCAN_START folders=%zu", dirs.size());
    for(const auto& d:dirs)mesen_diag::write("SCAN_FOLDER %s", d.c_str());
    auto listed=snes::Scan(dirs);
    mesen_diag::write("SCAN_COMPLETE games=%zu failed=%d",listed.size(),int(snes::ScanFailed()));
''' + shelf_body[cache_end:])
    shelf_body = shelf_body.replace('#include "scan_progress.h"',
        '#include "scan_progress.h"\\n#include "rom_diagnostics.h"'.replace('\\n',chr(10)))
    shelf.write_text(shelf_body)
    # Always explain why a directory was skipped: unmounted USB is
    # expected, while the two actual NES folders must be readable.
    scan_text=rom.read_text()
    opener='DIR* dir=opendir(path.c_str());if(!dir){const int code=errno;'
    assert opener in scan_text
    scan_text=scan_text.replace(opener,
        'DIR* dir=opendir(path.c_str());if(!dir){const int code=errno;' +
        'mesen_diag::write("DIR_FAIL path=%s errno=%d",path.c_str(),code);')
    after='    unsigned matched=0,metadata_fallbacks=0,skipped=0;int metadata_error=0;'
    assert after in scan_text
    scan_text=scan_text.replace(after,
        '    mesen_diag::write("DIR_OPEN path=%s",path.c_str());' + chr(10) + after)
    entry='        ++scan_progress.entries;' + chr(10) + '        std::string child=path+"/"+entry->d_name;'
    assert entry in scan_text
    scan_text=scan_text.replace(entry,
        '        ++scan_progress.entries;' + chr(10) +
        '        if(scan_progress.entries<=120)mesen_diag::write("DIR_ENTRY parent=%s name=%s type=%u",path.c_str(),entry->d_name,unsigned(entry->d_type));' + chr(10) +
        '        std::string child=path+"/"+entry->d_name;')
    closing='    closedir(dir);' + chr(10) + '    std::printf("[library] local %s: matched='
    assert closing in scan_text
    scan_text=scan_text.replace(closing,
        '    mesen_diag::write("DIR_DONE path=%s matched=%u metadata_fallbacks=%u skipped=%u",path.c_str(),matched,metadata_fallbacks,skipped);' + chr(10) + closing)
    rom.write_text(scan_text)

    # Real ROM results are also written to the log, without stdout redirection.
    game_src=rom.read_text()
    needle = """auto dot=name.rfind('.');games.push_back({path,name.substr(0,dot),bytes});"""
    assert needle in game_src
    game_src=game_src.replace(needle, needle+
        'mesen_diag::write("ROM_FOUND %s bytes=%llu",path.c_str(),(unsigned long long)bytes);')
    game_src=game_src.replace('#include "games.h"',
        '#include "games.h"\\n#include "rom_diagnostics.h"'.replace('\\n',chr(10)))
    rom.write_text(game_src)
    # Dedicated low-level logger opens its file on every write, so logs
    # survive stdout/stderr redirection problems in PS5 libc.
    shutil.copy2(ROOT / "vendor/rom_diagnostics.h", source / "rom_diagnostics.h")
    loading = source / "loading.cpp"
    loading_src=loading.read_text()
    assert 'fonts.AddText(ui,"SNES",' in loading_src
    assert 'v2 modded by YAHBALLON' in loading_src
    loading_src=loading_src.replace('fonts.AddText(ui,"SNES",','fonts.AddText(ui,"MesenX",')
    loading_src=loading_src.replace('v2 modded by YAHBALLON','v0.1.11 by YAHBALLON')
    # Do not claim the games are ready while the console denies /data access.
    loading_src=loading_src.replace('#include <cmath>',
        '#include <cmath>' + chr(10) + '#include <dirent.h>' + chr(10) + '#include <cerrno>')
    loading_src=loading_src.replace('    fonts.AddText(ui,done?"Games ready":"Loading games",',
        '''    bool denied=false;
    if(done && progress.games==0){
        DIR* probe=opendir("/data/PS5NES/games");
        if(probe)closedir(probe);
        else denied=(errno==EPERM||errno==EACCES);
    }
    fonts.AddText(ui,denied?"Filesystem access denied":(done?"Games ready":"Loading games"),''')
    loading_src=loading_src.replace('    fonts.AddText(ui,location.c_str(),',
        '''    if(denied)location="Elevation unavailable. Check lapy.elf / elfldr port 9021.";
    fonts.AddText(ui,location.c_str(),''')
    loading.write_text(loading_src)

    # Dedicated game-image shader: the generic signed-distance-field UI
    # shader uses dynamic branches that are unreliable for RGBA video textures
    # on the PS5 shader backend. Fonts still use their original R8 shader.
    shader = OUTPUT / "ps5/frontend/shaders/game.frag"
    shutil.copy2(ROOT / "vendor/mesen_game.frag", shader)
    renderer = OUTPUT / "ps5/frontend/fe_renderer.cpp"
    renderer_h = OUTPUT / "ps5/frontend/fe_renderer.h"
    replace_required(renderer_h, 'uint32_t width = 0, height = 0;',
        'uint32_t width = 0, height = 0;' + chr(10) +
        '    VkFormat format = VK_FORMAT_UNDEFINED;')
    replace_required(renderer_h, 'VkPipeline m_ui = VK_NULL_HANDLE;',
        'VkPipeline m_ui = VK_NULL_HANDLE;' + chr(10) +
        '    VkPipeline m_game_ui = VK_NULL_HANDLE;')
    replace_required(renderer,
        'const PipelineDesc ui = {fe_spv_ui_vert, sizeof(fe_spv_ui_vert), fe_spv_ui_frag, sizeof(fe_spv_ui_frag),',
        'const PipelineDesc game_ui = {fe_spv_ui_vert, sizeof(fe_spv_ui_vert), fe_spv_game_frag, sizeof(fe_spv_game_frag),' +
        chr(10) + '        &ui_binding, 1, ui_attrs, 4, PipelineDesc::Alpha, false, false, m_final_pass};' + chr(10) +
        '    const PipelineDesc ui = {fe_spv_ui_vert, sizeof(fe_spv_ui_vert), fe_spv_ui_frag, sizeof(fe_spv_ui_frag),')
    replace_required(renderer, 'make(post, m_post) && make(ui, m_ui);',
        'make(post, m_post) && make(ui, m_ui) && make(game_ui, m_game_ui);')
    replace_required(renderer, 'Texture* t = new Texture();',
        'Texture* t = new Texture();' + chr(10) + '    t->format = format;')
    replace_required(renderer,
        'm_vk->vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ui);',
        '''// RGBA game video bypasses the SDF/text branch entirely.
            const bool is_game = m_atlas && m_atlas->format == VK_FORMAT_R8G8B8A8_UNORM;
            m_vk->vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                is_game ? m_game_ui : m_ui);''')
    replace_required(renderer, '&m_post, &m_ui})','&m_post, &m_ui, &m_game_ui})')
    # Always regenerate SPIR-V after installing game.frag. The pinned
    # frontend's checked-in fe_shaders.inc would otherwise stay stale.
    subprocess.run(["bash", str(OUTPUT / "ps5/frontend/build_shaders.sh")],
        cwd=OUTPUT / "ps5/frontend", check=True)
    assert 'fe_spv_game_frag' in (OUTPUT / "ps5/frontend/fe_shaders.inc").read_text()
    assert 'is_game ? m_game_ui : m_ui' in renderer.read_text()

    # Remove SNES-only special-chip switches that cannot affect an NES core.
    options_file = OUTPUT / "ps5/frontend/fe_options.cpp"
    old_choices = '''{"Emulation", {Seg("SNES/Region", "Region", "auto", "", {{"auto","Automatic"},{"ntsc","NTSC"},{"pal","PAL"}}),
                       Seg("SNES/ChipMode", "Special-chip emulation", "HLE", "", {{"HLE","High-level"},{"LLE","Firmware-based"}})}, kTabSettings},'''
    assert old_choices in options_file.read_text()
    replace_required(options_file, old_choices,
        '''{"Emulation", {Toggle("NES/HdPacks", "HD Packs", "false", "", "Load Mesen hires textures from HdPacks/<ROM name>/hires.txt. Restart the game to apply.", true)}, kTabSettings},''')
    replace_required(options_file, 'Seg("SNES/Aspect",', 'Seg("NES/Aspect",')
    replace_required(options_file, 'Toggle("SNES/Sound",', 'Toggle("NES/Sound",')

    runtime = source / "shelf_runtime.cpp"
    replace_required(runtime, '''    core.SetOption("bsnes_region",get("SNES/Region","auto"));
    core.SetOption("bsnes_violate_accuracy","enabled");
    core.SetOption("bsnes_chip_hle",get("SNES/ChipMode","HLE"));
    core.SetOption("bsnes_gamma_ramp",get("SNES/Gamma","disabled"));''',
        '''    const std::string hd_choice=get("NES/HdPacks","false");
    const bool hd_on=(hd_choice=="true"||hd_choice=="enabled");
    core.SetOption("mesen_hdpacks",hd_on?"enabled":"disabled");
    mesen_diag::write("HDPACK_SETTING game=%s ui=%s core=%s",path.c_str(),hd_choice.c_str(),hd_on?"enabled":"disabled");''')
    replace_required(runtime, 'get("SNES/Aspect","4:3")', 'get("NES/Aspect","4:3")')
    replace_required(runtime, 'get("SNES/Sound","true")', 'get("NES/Sound","true")')
    replace_required(runtime, '"SNES audio port failed"', '"MesenCE audio port failed"')
    replace_required(runtime, '"SNES frame upload failed"', '"MesenCE frame upload failed"')
    assert 'hd_choice=="true"||hd_choice=="enabled"' in runtime.read_text()
    assert 'hd_on?"enabled":"disabled"' in runtime.read_text()
    assert 'Toggle("NES/HdPacks", "HD Packs", "false"' in options_file.read_text()

    # Isolated NES cache: the source inherited a SNES-only extension check
    # and otherwise refuses to read valid .nes entries after the first scan.
    cache = source / "library_cache.cpp"
    replace_required(cache, 'PS5SNES4\\0', 'PS5MXN01\\0')
    replace_required(cache,
        'if(extension!=".zip"&&extension!=".sfc"&&extension!=".smc"&&extension!=".swc"&&extension!=".fig")return false;',
        'if(extension!=".zip"&&extension!=".nes")return false;')

    # Home directory also owns MesenCE HdPacks/<ROM stem>/hires.txt.
    main = source / "main_shelf.cpp"
    replace_required(main, 'PS5SNES', TITLE)
    replace_required(main, 'PPSA99205', TITLE_ID)
    replace_required(main, '#include <cerrno>', '#include <cerrno>' + chr(10) + '#include <dirent.h>')
    replace_required(main, '"/SNES"', '"/NES"')
    # Share ONLY the ROM library with PS5NES. Keep saves, cover cache,
    # HD packs and logs in /data/PS5MesenX to avoid clobbering other apps.
    replace_required(main, 'paths.games_dir=root+"/games";',
                     'paths.games_dir="/data/PS5NES/games";')
    replace_required(main,
                     '    OrbisFrontendPaths paths;paths.top_dir=root;',
                     '''    // A shared ROM directory must exist even when PS5NES is not installed.
    if (mkdir("/data/PS5NES",0777)!=0 && errno!=EEXIST)
        std::printf("[games] could not create /data/PS5NES (errno=%d)\\n",errno);
    if (mkdir("/data/PS5NES/games",0777)!=0 && errno!=EEXIST)
        std::printf("[games] could not create /data/PS5NES/games (errno=%d)\\n",errno);
    std::printf("[games] shared NES ROM directory: /data/PS5NES/games\\n");
    OrbisFrontendPaths paths;paths.top_dir=root;''')
    # Also replace the old games folder in the supplemental scan list,
    # or the frontend could show duplicate ROM entries.
    replace_required(main, '"/data/PS5MesenX/games","/data/PS5MesenX/roms"',
                     '"/data/PS5MesenX/roms"')

    replace_required(main, 'for(const char* dir:{"games","saves","settings","covers","cache","logs"})',
                    'for(const char* dir:{"games","saves","settings","covers","cache","logs","HdPacks","SaveStates","Saves"})')

    # Retain the two user-provided paths and our original ROM folder. The
    # regular scanner performs path-based deduplication for overlapping roots.
    replace_required(main,
        '"/data/PS5MesenX/roms","/data/SNES"',
        '"/data/PS5MesenX/games","/data/PS5MesenX/roms","/data/NES"')
    replace_required(main, '    orbis_frontend_set_language(root+"/lang");',
        '''    // Inspect actual filesystem visibility before scanner thread starts.
    for(const char* folder:{"/data/PS5NES/games","/data/homebrew/PPSA99896/roms",
                            "/data/homebrew/PPSA99896/games","/data/PS5MesenX/games"}) {
        DIR* dir=opendir(folder);
        if(!dir) {
            std::printf("[romscan] inaccessible %s errno=%d\\n",folder,errno);
            continue;
        }
        int count=0;
        while(auto* entry=readdir(dir)) {
            if(entry->d_name[0]=='.')continue;
            if(count<12)std::printf("[romscan] entry %s/%s\\n",folder,entry->d_name);
            ++count;
        }
        closedir(dir);
        std::printf("[romscan] %s: %d visible entries\\n",folder,count);
    }
    orbis_frontend_set_language(root+"/lang");''')

    # Log initial elevation, active root and filesystem access even when
    # stdout is unavailable. This is independent of the normal console logger.
    replace_required(main, '#include <dirent.h>',
                     '#include <dirent.h>' + chr(10) + '#include "rom_diagnostics.h"')
    replace_required(main, '    sceNetInit();',
                     '    mesen_diag::write("BOOT version=0.1.7 title=PPSA99896");' +
                     chr(10) + '    sceNetInit();')
    replace_required(main, '    std::string root="/data/PS5MesenX";',
                     '    mesen_diag::write("ELEVATION filesystem=%u path=%s",unsigned(access),elevation::path());' +
                     chr(10) + '    if(access!=elevation::Status::ok)mesen_diag::write("FILESYSTEM_DENIED: check /app0/lapy.elf, resident Lapy or elfldr 9021");' +
                     chr(10) + '    std::string root="/data/PS5MesenX";')
    replace_required(main, '    int fd=open((root+"/logs/boot.log").c_str()',
                     '    mesen_diag::write("ROOT %s",root.c_str());' +
                     chr(10) + '    int fd=open((root+"/logs/boot.log").c_str()')

    # The initial boot logger wrote BOOT/ELEVATION before stdout was opened,
    # but O_TRUNC erased those diagnostics on startup. Preserve all lines.
    replace_required(main, 'O_WRONLY|O_CREAT|O_TRUNC|O_APPEND,0666',
                     'O_WRONLY|O_CREAT|O_APPEND,0666')
    replace_required(main, '    orbis_frontend_set_language(root+"/lang");',
                     '    mesen_diag::write("SCAN_READY root=%s games=/data/PS5NES/games",root.c_str());' +
                     chr(10) + '    orbis_frontend_set_language(root+"/lang");')

    # The SNES frontend cache paths and per-ROM NFS root must be independent.
    replace_required(shelf, 'PS5SNES', TITLE)
    for name in ("INSTALL.md",):
        f = OUTPUT / "snes" / name
        if f.exists():
            f.write_text(f.read_text().replace("PS5SNES", TITLE).replace("PPSA99205", TITLE_ID))

    param_path = OUTPUT / "snes/sce_sys/param.json"
    param = json.loads(param_path.read_text())
    param["titleId"] = TITLE_ID
    param["conceptId"] = TITLE_ID.replace("PPSA", "")
    param["contentId"] = f"UP9000-{TITLE_ID}_00-PS5MESENX0000001"
    param["localizedParameters"]["en-US"]["titleName"] = TITLE
    param["contentVersion"] = "01.000.012"
    param_path.write_text(json.dumps(param, indent=2) + "\n")
    for file in (rom, shelf, runtime, main):
        if "PS5SNES" in file.read_text() or "bsnes_" in file.read_text():
            raise RuntimeError(f"Legacy SNES app or core identifier in {file}")
    assert "NES\\x1A" in rom.read_text()
    assert 'v0.1.11 by YAHBALLON' in loading.read_text()
    assert 'SCAN_COMPLETE' in shelf.read_text()
    assert 'Filesystem access denied' in loading.read_text()
    assert 'elevation::path()' in main.read_text()
    assert 'snes::ReportCached(' not in shelf.read_text()
    assert 'mesen_diag::write("ROM_FOUND' in rom.read_text()
    assert 'DIR_OPEN path=%s' in rom.read_text()
    assert 'DIR_FAIL path=%s' in rom.read_text()
    assert 'O_WRONLY|O_CREAT|O_TRUNC|O_APPEND' not in main.read_text()
    # Fail the build early if the shared NES path is not actually wired.
    compiled_main=main.read_text()
    assert 'paths.games_dir="/data/PS5NES/games";' in compiled_main
    assert 'mkdir("/data/PS5NES/games",0777)' in compiled_main
    assert '"/data/homebrew/PPSA99896/roms"' in compiled_main
    assert '"/data/PS5NES/games"' in compiled_main
    assert "[romscan]" in compiled_main
    assert '#include <cerrno>' + chr(10) + '#include <dirent.h>' in compiled_main
    assert 'extension!=".zip"&&extension!=".nes"' in cache.read_text()
    assert 'PS5MXN01\\0' in cache.read_text()
    assert 'readable_header' in rom.read_text()
    assert 'ROM metadata unavailable; bounded stream read' in rom.read_text()
    print("Prepared PS5MesenX source under", OUTPUT)
    print("NOTE: Core build/link not implemented in this preparation script.")

if __name__ == "__main__":
    generate()
