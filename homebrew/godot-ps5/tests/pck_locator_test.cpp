#include "../engine/platform/ps5/pck_locator.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unistd.h>

int main() {
    char pattern[]="/tmp/godotps5-XXXXXX";
    const char *dir=mkdtemp(pattern);
    assert(dir);
    const std::string one=std::string(dir)+"/a.pck",two=std::string(dir)+"/b.PCK";
    const std::string bad=std::string(dir)+"/c.pck";
    const unsigned char header[]={'G','D','P','C',2,0,0,0,4,0,0,0,7,0,0,0,0,0,0,0};
    auto save=[](const std::string &name,const unsigned char *data,size_t size) {
        FILE *f=fopen(name.c_str(),"wb"); assert(f);
        assert(fwrite(data,1,size,f)==size); fclose(f);
    };
    save(two,header,sizeof(header));save(one,header,sizeof(header));
    unsigned char corrupt[sizeof(header)];std::memcpy(corrupt,header,sizeof(header));
    corrupt[8]=3;save(bad,corrupt,sizeof(corrupt));
    assert(godot_ps5::pack_name("a.PCK"));
    assert(!godot_ps5::pack_name("a.pck.bak"));
    assert(godot_ps5::valid_godot4_pack(one));
    assert(!godot_ps5::valid_godot4_pack(bad));
    assert(godot_ps5::first_pack_in(dir)==one);
    assert(godot_ps5::choose_pack(one.c_str())==one);
    assert(godot_ps5::choose_pack(bad.c_str()).empty());
    assert(godot_ps5::choose_pack("../a.pck").empty());
    const auto link=std::string(dir)+"/link.pck";
    assert(symlink(one.c_str(),link.c_str())==0);
    assert(!godot_ps5::valid_godot4_pack(link));
    unlink(link.c_str());unlink(one.c_str());unlink(two.c_str());unlink(bad.c_str());rmdir(dir);
    puts("PASS Godot 4 PCK locator");
}
