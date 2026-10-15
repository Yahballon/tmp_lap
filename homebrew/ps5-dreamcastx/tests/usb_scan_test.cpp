#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <vector>
#include "../src/dreamcastx_usb.inc"

int main()
{
    namespace fs=std::filesystem;
    char tmp[]="/tmp/ps5dreamcastx-usb-XXXXXX";
    char* root=mkdtemp(tmp);
    assert(root);
    const std::string prefix=std::string(root)+"/usb";
    fs::create_directories(prefix+"0/PS5DreamcastX/games");
    fs::create_directories(prefix+"1/PS5DreamcastX");
    fs::create_directories(prefix+"2/Dreamcast");
    fs::create_directories(prefix+"3/unrelated");
    fs::create_directories(prefix+"4/ps5dreamcastx/games");
    auto dirs=discoverDreamcastXUsbDirs(prefix,8);
    std::sort(dirs.begin(),dirs.end());
    assert(dirs.size()==4);
    assert(std::find(dirs.begin(),dirs.end(),prefix+"0/PS5DreamcastX/games")!=dirs.end());
    assert(std::find(dirs.begin(),dirs.end(),prefix+"1/PS5DreamcastX")!=dirs.end());
    assert(std::find(dirs.begin(),dirs.end(),prefix+"2/Dreamcast")!=dirs.end());
    assert(std::find(dirs.begin(),dirs.end(),prefix+"4/ps5dreamcastx/games")!=dirs.end());
    fs::remove_all(root);
    puts("PASS: USB0-7, app name, legacy Flycast folders, root/games and case-insensitive names");
}
